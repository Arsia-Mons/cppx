// Hello-world demo: Clay UI + a minimal React-style runtime.
//
// What this shows:
//   - useState        : Counter's `count` survives across frames.
//   - useEffect       : "Counter mounted/unmounted" logged on lifecycle.
//   - PROVIDE/useContext : App provides a Theme; Counter reads it for color.
//
// Controls:
//   UP / DOWN : increment / decrement the counter
//   T         : cycle theme (App owns the index via useState, propagated via Provider)
//   M         : mount/unmount the Counter component (triggers effect cleanup)
//   I         : fetch a new random image (worker thread; cleanup-on-unmount)
//   ESC       : quit

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

#include <clay.h>
#include <clay_renderer_SDL3.h>

#include "react.h"
#include "stb_image.h"

#include <curl/curl.h>

#include <atomic>
#include <vector>

#include <stdio.h>
#include <string.h>

// ----------------------------------------------------------------------------
// Globals: SDL/Clay/font + per-frame input edges.
// ----------------------------------------------------------------------------

static SDL_Window         *g_window    = nullptr;
static SDL_Renderer       *g_sdl       = nullptr;
static TTF_TextEngine     *g_text_eng  = nullptr;
static TTF_Font           *g_font      = nullptr;
static Clay_SDL3RendererData g_clay_rd = {};

static struct {
    bool up;
    bool down;
    bool m;
    bool t;
    bool i;
} g_edges;  // single-frame "just pressed" edges

static bool g_show_counter = true;

// ----------------------------------------------------------------------------
// Font discovery + Clay text measurement.
// ----------------------------------------------------------------------------

static TTF_Font *open_some_font(float ptsize) {
    const char *candidates[] = {
        "/System/Library/Fonts/Helvetica.ttc",
        "/System/Library/Fonts/Supplemental/Arial.ttf",
        "/Library/Fonts/Arial.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/TTF/DejaVuSans.ttf",
        "C:\\Windows\\Fonts\\arial.ttf",
    };
    for (const char *p : candidates) {
        TTF_Font *f = TTF_OpenFont(p, ptsize);
        if (f) {
            SDL_Log("loaded font: %s", p);
            return f;
        }
    }
    return nullptr;
}

static Clay_Dimensions measure_text(Clay_StringSlice text,
                                    Clay_TextElementConfig *cfg,
                                    void *user_data) {
    (void)user_data;
    TTF_Font *font = g_font;
    if (!font) return { 0, 0 };
    TTF_SetFontSize(font, cfg->fontSize);
    int w = 0, h = 0;
    TTF_GetStringSize(font, text.chars, (size_t)text.length, &w, &h);
    return { (float)w, (float)h };
}

static void on_clay_error(Clay_ErrorData err) {
    fprintf(stderr, "clay: %.*s\n", (int)err.errorText.length, err.errorText.chars);
}

// ----------------------------------------------------------------------------
// React-style components.
// ----------------------------------------------------------------------------

struct Theme {
    const char *name;
    Clay_Color  fg;
    Clay_Color  bg;
    Clay_Color  panel;
};

static Theme g_themes[] = {
    { "Dark",    {220, 230, 255, 255}, { 12,  14,  22, 255}, { 28,  32,  48, 255} },
    { "Light",   { 30,  30,  40, 255}, {245, 245, 250, 255}, {220, 225, 235, 255} },
    { "Sunset",  {255, 240, 210, 255}, { 30,  15,  40, 255}, { 80,  30,  60, 255} },
};
static const int g_theme_count = sizeof(g_themes) / sizeof(g_themes[0]);

static ReactContext ThemeContext = {};

static void on_counter_mount(void *user) {
    (void)user;
    SDL_Log("[effect] Counter mounted");
}

static void on_counter_unmount(void *user) {
    (void)user;
    SDL_Log("[effect] Counter unmounted (cleanup ran)");
}

static Clay_String cs(const char *s) {
    return Clay_String{ false, (int32_t)strlen(s), s };
}

static void Counter(void) {
    REACT_COMPONENT_BEGIN("Counter") {
        // --- hooks (always in this order; never conditional) ---
        int *count = use_state_int(0);
        use_effect(on_counter_mount, on_counter_unmount, /*user*/ nullptr, /*deps*/ 0);
        Theme *theme = (Theme *)use_context(&ThemeContext);

        // --- input → state (would be a setter in real React) ---
        if (g_edges.up)   *count += 1;
        if (g_edges.down) *count -= 1;

        // --- format the dynamic text into a buffer that outlives Clay_EndLayout ---
        static char count_buf[64];
        snprintf(count_buf, sizeof(count_buf), "Count: %d", *count);

        CLAY({
            .id = CLAY_ID_LOCAL("CounterPanel"),
            .layout = {
                .sizing = { CLAY_SIZING_FIT(0), CLAY_SIZING_FIT(0) },
                .padding = CLAY_PADDING_ALL(20),
                .childGap = 8,
                .layoutDirection = CLAY_TOP_TO_BOTTOM,
            },
            .backgroundColor = theme->panel,
            .cornerRadius = CLAY_CORNER_RADIUS(8),
        }) {
            CLAY_TEXT(cs(count_buf),
                CLAY_TEXT_CONFIG({ .textColor = theme->fg, .fontSize = 32 }));
            CLAY_TEXT(cs("(UP/DOWN to change)"),
                CLAY_TEXT_CONFIG({ .textColor = theme->fg, .fontSize = 14 }));
        }
    } REACT_COMPONENT_END();
}

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

static void Image(void) {
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

        CLAY({
            .id = CLAY_ID_LOCAL("ImagePanel"),
            .layout = {
                .sizing = { CLAY_SIZING_FIXED(220), CLAY_SIZING_FIT(0) },
                .padding = CLAY_PADDING_ALL(10),
                .childGap = 6,
                .childAlignment = { CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER },
                .layoutDirection = CLAY_TOP_TO_BOTTOM,
            },
            .backgroundColor = theme->panel,
            .cornerRadius = CLAY_CORNER_RADIUS(8),
        }) {
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
        }
    } REACT_COMPONENT_END();
}

static void App(void) {
    REACT_COMPONENT_BEGIN("App") {
        // --- hooks ---
        int *theme_idx = use_state_int(0);

        // --- input → state ---
        if (g_edges.m) g_show_counter = !g_show_counter;
        if (g_edges.t) *theme_idx = (*theme_idx + 1) % g_theme_count;

        Theme *theme = &g_themes[*theme_idx];

        static char hint_buf[128];
        snprintf(hint_buf, sizeof(hint_buf),
                 "T: theme (%s)   M: toggle Counter   I: fetch image   ESC: quit",
                 theme->name);

        PROVIDE(&ThemeContext, theme) {
            CLAY({
                .id = CLAY_ID_LOCAL("Root"),
                .layout = {
                    .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) },
                    .padding = CLAY_PADDING_ALL(32),
                    .childGap = 16,
                    .layoutDirection = CLAY_TOP_TO_BOTTOM,
                },
                .backgroundColor = theme->bg,
            }) {
                CLAY_TEXT(cs("Hello, World!"),
                    CLAY_TEXT_CONFIG({ .textColor = theme->fg, .fontSize = 40 }));
                CLAY_TEXT(cs(hint_buf),
                    CLAY_TEXT_CONFIG({ .textColor = theme->fg, .fontSize = 14 }));

                CLAY({
                    .id = CLAY_ID_LOCAL("ContentRow"),
                    .layout = {
                        .sizing = { CLAY_SIZING_FIT(0), CLAY_SIZING_FIT(0) },
                        .childGap = 16,
                        .layoutDirection = CLAY_LEFT_TO_RIGHT,
                    },
                }) {
                    if (g_show_counter) {
                        Counter();
                    }
                    Image();
                }
            }
        }
    } REACT_COMPONENT_END();
}

// ----------------------------------------------------------------------------
// SDL bootstrap + main loop.
// ----------------------------------------------------------------------------

int main(int, char **) {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
        return 1;
    }
    if (!TTF_Init()) {
        fprintf(stderr, "TTF_Init: %s\n", SDL_GetError());
        return 1;
    }
    curl_global_init(CURL_GLOBAL_DEFAULT);

    g_window = SDL_CreateWindow("clay + react hello-world", 800, 500, SDL_WINDOW_RESIZABLE);
    if (!g_window) { fprintf(stderr, "SDL_CreateWindow: %s\n", SDL_GetError()); return 1; }
    g_sdl = SDL_CreateRenderer(g_window, nullptr);
    if (!g_sdl) { fprintf(stderr, "SDL_CreateRenderer: %s\n", SDL_GetError()); return 1; }
    SDL_SetRenderVSync(g_sdl, 1);

    g_text_eng = TTF_CreateRendererTextEngine(g_sdl);
    g_font = open_some_font(16.0f);
    if (!g_font) {
        fprintf(stderr, "no font found; tried system defaults\n");
        return 1;
    }
    TTF_Font *fonts[1] = { g_font };
    g_clay_rd.renderer   = g_sdl;
    g_clay_rd.textEngine = g_text_eng;
    g_clay_rd.fonts      = fonts;

    // --- Clay init ---
    uint32_t clay_mem = Clay_MinMemorySize();
    void    *clay_buf = SDL_malloc(clay_mem);
    Clay_Arena clay_arena = Clay_CreateArenaWithCapacityAndMemory(clay_mem, clay_buf);
    int win_w = 800, win_h = 500;
    SDL_GetWindowSize(g_window, &win_w, &win_h);
    Clay_Context *clay_ctx = Clay_Initialize(
        clay_arena,
        Clay_Dimensions{ (float)win_w, (float)win_h },
        Clay_ErrorHandler{ on_clay_error, 0 });
    Clay_SetMeasureTextFunction(measure_text, nullptr);

    react_init(clay_ctx);

    // --- main loop ---
    bool running = true;
    while (running) {
        // Reset per-frame input edges.
        g_edges = {};

        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            switch (ev.type) {
                case SDL_EVENT_QUIT:
                    running = false; break;
                case SDL_EVENT_KEY_DOWN:
                    if (ev.key.repeat) break;
                    switch (ev.key.key) {
                        case SDLK_ESCAPE: running = false; break;
                        case SDLK_UP:     g_edges.up   = true; break;
                        case SDLK_DOWN:   g_edges.down = true; break;
                        case SDLK_M:      g_edges.m    = true; break;
                        case SDLK_T:      g_edges.t    = true; break;
                        case SDLK_I:      g_edges.i    = true; break;
                        default: break;
                    }
                    break;
                case SDL_EVENT_WINDOW_RESIZED:
                    Clay_SetLayoutDimensions(Clay_Dimensions{
                        (float)ev.window.data1, (float)ev.window.data2 });
                    break;
                default: break;
            }
        }

        // Pointer (not strictly needed but Clay likes a fresh state each frame).
        float mx, my;
        Uint32 mb = SDL_GetMouseState(&mx, &my);
        Clay_SetPointerState(Clay_Vector2{ mx, my }, (mb & SDL_BUTTON_LMASK) != 0);

        // --- build the UI tree (React render phase) ---
        react_begin_frame();
        Clay_BeginLayout();
        App();
        Clay_RenderCommandArray cmds = Clay_EndLayout();  // commit
        react_end_frame();                                // run effects, sweep unmounts

        // --- draw ---
        SDL_SetRenderDrawColor(g_sdl, 12, 14, 22, 255);
        SDL_RenderClear(g_sdl);
        SDL_Clay_RenderClayCommands(&g_clay_rd, &cmds);
        SDL_RenderPresent(g_sdl);
    }

    SDL_free(clay_buf);
    TTF_CloseFont(g_font);
    TTF_DestroyRendererTextEngine(g_text_eng);
    SDL_DestroyRenderer(g_sdl);
    SDL_DestroyWindow(g_window);
    TTF_Quit();
    SDL_Quit();
    curl_global_cleanup();
    return 0;
}
