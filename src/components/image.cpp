#include "image.h"

#include <clay.h>

#include "../app_state.h"
#include "../react.h"

#include "panel.h"

#include <SDL3/SDL.h>
#include <curl/curl.h>

#include "stb_image.h"

#include <atomic>
#include <vector>

// ----------------------------------------------------------------------------
// Async image fetch: worker thread + refcounted shared cell.
//
// The cell is the small heap-allocated struct that both the main thread and
// the worker thread can see. Refcounting keeps it alive across an unmount
// while the worker is still running.
// ----------------------------------------------------------------------------

struct FetchCell {
    std::atomic<int>  ref;        // refcount: main + worker = 2 while in flight
    std::atomic<bool> cancelled;  // main thread sets on cleanup
    std::atomic<bool> ready;      // worker sets when rgba is populated
    int               w = 0, h = 0;
    unsigned char    *rgba = nullptr;   // owned by worker until ready; then by main
    SDL_Texture      *texture = nullptr; // main-thread only
};

static void cell_retain(FetchCell *c) {
    c->ref.fetch_add(1, std::memory_order_relaxed);
}
static void cell_release(FetchCell *c) {
    if (c->ref.fetch_sub(1, std::memory_order_acq_rel) == 1) {
        if (c->rgba) stbi_image_free(c->rgba);
        // texture should already have been destroyed by main thread; if not
        // (worker holds last ref), it was never created, so nothing to do.
        delete c;
    }
}

static size_t curl_write_to_vec(void *ptr, size_t size, size_t n, void *user) {
    auto &vec = *static_cast<std::vector<unsigned char> *>(user);
    size_t total = size * n;
    vec.insert(vec.end(), (unsigned char *)ptr, (unsigned char *)ptr + total);
    return total;
}

static int curl_check_cancel(void *user, curl_off_t, curl_off_t, curl_off_t, curl_off_t) {
    return static_cast<FetchCell *>(user)->cancelled.load(std::memory_order_relaxed) ? 1 : 0;
}

static int worker_fetch(void *user) {
    FetchCell *c = static_cast<FetchCell *>(user);
    if (c->cancelled.load(std::memory_order_relaxed)) {
        cell_release(c);
        return 0;
    }

    std::vector<unsigned char> body;
    CURL *curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, "https://picsum.photos/200/200");
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_to_vec);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &body);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15L);
        // Real cancellation: progress callback returns non-zero → libcurl
        // aborts the transfer with CURLE_ABORTED_BY_CALLBACK (within ~100ms
        // of the cancelled flag flipping).
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
        curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, curl_check_cancel);
        curl_easy_setopt(curl, CURLOPT_XFERINFODATA, c);
        CURLcode rc = curl_easy_perform(curl);
        curl_easy_cleanup(curl);

        if (rc == CURLE_OK && !body.empty() &&
            !c->cancelled.load(std::memory_order_relaxed)) {
            int w, h, comp;
            unsigned char *rgba = stbi_load_from_memory(
                body.data(), (int)body.size(), &w, &h, &comp, 4);
            if (rgba) {
                if (c->cancelled.load(std::memory_order_relaxed)) {
                    stbi_image_free(rgba);
                } else {
                    c->rgba = rgba;
                    c->w = w;
                    c->h = h;
                    c->ready.store(true, std::memory_order_release);
                }
            }
        }
    }
    cell_release(c);
    return 0;
}

static void start_fetch(void *user) {
    FetchCell *c = static_cast<FetchCell *>(user);
    if (!c) return;
    cell_retain(c); // worker's ref
    SDL_Thread *t = SDL_CreateThread(worker_fetch, "img-fetch", c);
    if (!t) {
        SDL_Log("SDL_CreateThread failed: %s", SDL_GetError());
        cell_release(c); // worker never started; undo retain
    } else {
        SDL_DetachThread(t);
    }
}

static void cancel_fetch(void *user) {
    FetchCell *c = static_cast<FetchCell *>(user);
    if (!c) return;
    c->cancelled.store(true, std::memory_order_relaxed);
    if (c->texture) {
        SDL_DestroyTexture(c->texture);
        c->texture = nullptr;
    }
    cell_release(c);
}

void Image(const ReactNoProps &props) {
    (void)props;
    REACT_COMPONENT_BEGIN("Image") {
        Theme    *theme    = (Theme *)use_context(&ThemeContext);
        int      *seq      = use_state_int(0);
        void    **cell_ref = use_ref(nullptr);

        // 'I' edge: bump seq and create a new cell. The deps change will cause
        // use_effect's cleanup to fire on the OLD cell (cancel + release),
        // then start_fetch on the NEW one.
        if (g_edges.i) {
            *seq += 1;
            FetchCell *nc = new FetchCell();
            nc->ref.store(1, std::memory_order_relaxed); // component's ref
            *cell_ref = nc;
        }

        FetchCell *cell = static_cast<FetchCell *>(*cell_ref);
        use_effect(start_fetch, cancel_fetch, cell, (uint64_t)*seq);

        // If the worker has delivered bytes but we haven't uploaded yet,
        // create the SDL_Texture now (main thread only) and free the rgba.
        if (cell && cell->ready.load(std::memory_order_acquire) &&
            !cell->texture && cell->rgba) {
            SDL_Surface *surf = SDL_CreateSurfaceFrom(
                cell->w, cell->h, SDL_PIXELFORMAT_RGBA32,
                cell->rgba, cell->w * 4);
            if (surf) {
                cell->texture = SDL_CreateTextureFromSurface(g_sdl, surf);
                SDL_DestroySurface(surf);
            }
            stbi_image_free(cell->rgba);
            cell->rgba = nullptr;
        }

        const char *status =
            !cell                        ? "press I to fetch"
          : cell->texture                ? ""
          : cell->cancelled.load()       ? "cancelled"
                                         : "loading...";

        PanelProps panel = {
            .id = CLAY_ID_LOCAL("ImagePanel"),
            .sizing = { CLAY_SIZING_FIXED(220), CLAY_SIZING_FIT(0) },
            .padding = CLAY_PADDING_ALL(10),
            .child_gap = 6,
            .child_alignment = { CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER },
            .direction = CLAY_TOP_TO_BOTTOM,
            .background = theme->panel,
            .radius = CLAY_CORNER_RADIUS(8),
        };
        Panel(panel, [&] {
            if (cell && cell->texture) {
                CLAY({
                    .id = CLAY_ID_LOCAL("ImageContent"),
                    .layout = { .sizing = { CLAY_SIZING_FIXED(200), CLAY_SIZING_FIXED(200) } },
                    .image = { .imageData = cell->texture },
                });
            } else {
                CLAY({
                    .id = CLAY_ID_LOCAL("ImagePlaceholder"),
                    .layout = {
                        .sizing = { CLAY_SIZING_FIXED(200), CLAY_SIZING_FIXED(200) },
                        .childAlignment = { CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER },
                    },
                    .backgroundColor = theme->bg,
                    .cornerRadius = CLAY_CORNER_RADIUS(4),
                }) {
                    CLAY_TEXT(cs(status),
                        CLAY_TEXT_CONFIG({ .textColor = theme->fg, .fontSize = 14 }));
                }
            }
            CLAY_TEXT(cs("(I to fetch)"),
                CLAY_TEXT_CONFIG({ .textColor = theme->fg, .fontSize = 12 }));
        });
    } REACT_COMPONENT_END();
}
