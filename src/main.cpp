// Hello-world demo: Clay UI + a minimal React-style runtime.
//
// What this shows:
//   - useState        : Counter/theme state survives across frames.
//   - useEffect       : "Counter mounted/unmounted" logged on lifecycle.
//   - PROVIDE/useContext : App provides input; ThemeProvider provides theme.
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

#include "app_state.h"
#include "input.h"
#include "react.h"

#include "ui/components/app.h"

#include <curl/curl.h>

#include <stdio.h>

// ----------------------------------------------------------------------------
// Definitions for the shared globals declared in app_state.h.
// ----------------------------------------------------------------------------

SDL_Renderer *g_sdl         = nullptr;

// ----------------------------------------------------------------------------
// Local SDL/Clay/font state.
// ----------------------------------------------------------------------------

static SDL_Window           *g_window   = nullptr;
static TTF_TextEngine       *g_text_eng = nullptr;
static TTF_Font             *g_font     = nullptr;
static Clay_SDL3RendererData g_clay_rd  = {};

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
        InputState input = {};

        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            switch (ev.type) {
                case SDL_EVENT_QUIT:
                    running = false; break;
                case SDL_EVENT_KEY_DOWN:
                    if (ev.key.repeat) break;
                    switch (ev.key.key) {
                        case SDLK_ESCAPE: running = false; break;
                        case SDLK_UP:   input.increment_counter = true; break;
                        case SDLK_DOWN: input.decrement_counter = true; break;
                        case SDLK_M:    input.toggle_counter = true; break;
                        case SDLK_T:    input.cycle_theme = true; break;
                        case SDLK_I:    input.fetch_image = true; break;
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
        App(&input);
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
