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
//   ESC       : quit

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

#include <clay.h>
#include <clay_renderer_SDL3.h>

#include "react.h"

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

static void App(void) {
    REACT_COMPONENT_BEGIN("App") {
        // --- hooks ---
        int *theme_idx = use_state_int(0);

        // --- input → state ---
        if (g_edges.m) g_show_counter = !g_show_counter;
        if (g_edges.t) *theme_idx = (*theme_idx + 1) % g_theme_count;

        Theme *theme = &g_themes[*theme_idx];

        static char hint_buf[96];
        snprintf(hint_buf, sizeof(hint_buf),
                 "T: theme (%s)   M: toggle Counter   ESC: quit", theme->name);

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

                if (g_show_counter) {
                    Counter();
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
    return 0;
}
