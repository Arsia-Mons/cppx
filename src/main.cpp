// SDL platform shell + GameUiPipeline + the shooter UI example.
//
// What this shows:
//   - retained screens owned by ClientUi.
//   - hooks returning shooter values and write functions.
//   - focus, primitives, and post-layout write draining through GameUiPipeline.
//
// Controls:
//   UP / DOWN / LEFT / RIGHT : focus navigation
//   ENTER / SPACE            : confirm
//   ESC       : quit

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

#include <clay.h>
#include <clay_renderer_SDL3.h>

#include "app_state.h"
#include "game/ui/game_ui_pipeline.h"
#include "input.h"
#include "platform/control_mailbox.h"
#include "platform/input_adapter.h"
#include "react.h"
#include "shooter/shooter_game.h"
#include "shooter/shooter_ui.h"

#include <curl/curl.h>

#include <memory>
#include <sstream>
#include <stdio.h>
#include <string.h>
#include <string>

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

static std::string json_escape(const char *value) {
    std::string out;
    if (!value) return out;
    for (const char *p = value; *p; ++p) {
        switch (*p) {
            case '\\': out += "\\\\"; break;
            case '"': out += "\\\""; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default: out += *p; break;
        }
    }
    return out;
}

static std::string shooter_state_json(const shooter::ShooterGame &game) {
    std::ostringstream out;
    out << "{"
        << "\"health\":" << game.health() << ","
        << "\"armor\":" << game.armor() << ","
        << "\"ammo\":" << game.ammo() << ","
        << "\"credits\":" << game.credits() << ","
        << "\"selected_weapon\":" << game.selected_weapon() << ","
        << "\"compare_enabled\":" << (game.compare_enabled() ? "true" : "false")
        << ",\"weapons\":[";
    for (int i = 0; i < game.weapon_count(); ++i) {
        const shooter::WeaponState &weapon = game.weapon(i);
        if (i) out << ",";
        out << "{"
            << "\"index\":" << i << ","
            << "\"name\":\"" << json_escape(weapon.spec.name) << "\","
            << "\"role\":\"" << json_escape(weapon.spec.role) << "\","
            << "\"cost\":" << weapon.spec.cost << ","
            << "\"damage\":" << weapon.spec.damage << ","
            << "\"ammo\":" << weapon.spec.ammo << ","
            << "\"owned\":" << (weapon.owned ? "true" : "false") << ","
            << "\"equipped\":" << (weapon.equipped ? "true" : "false") << ","
            << "\"can_buy\":" << (game.can_buy_weapon(i) ? "true" : "false") << ","
            << "\"can_equip\":" << (game.can_equip_weapon(i) ? "true" : "false")
            << "}";
    }
    out << "]}";
    return out.str();
}

// ----------------------------------------------------------------------------
// SDL bootstrap + main loop.
// ----------------------------------------------------------------------------

int main(int argc, char **argv) {
    const char *control_dir = nullptr;
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--control-dir") == 0 && i + 1 < argc) {
            control_dir = argv[++i];
        }
    }

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
    SDL_SetRenderVSync(g_sdl, control_dir ? 0 : 1);

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

    game::ui::GameUiPipeline ui_pipeline;
    shooter::ShooterGame shooter_game;
    ui_pipeline.client_ui().push_screen(
        std::make_unique<shooter::ShooterGameScreen>(&shooter_game));
    platform::ControlMailbox control;
    control.set_game_state_json_provider([&shooter_game] {
        return shooter_state_json(shooter_game);
    });
    if (control_dir && !control.init(control_dir)) {
        return 1;
    }

    // --- main loop ---
    bool running = true;
    bool previous_pointer_down = false;
    while (running) {
        InputState input = {};
        ::ui::UiInputFrame ui_input = {};

        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            switch (ev.type) {
                case SDL_EVENT_QUIT:
                    running = false; break;
                case SDL_EVENT_KEY_DOWN:
                    if (ev.key.repeat) break;
                    platform::apply_key_down(ev.key.key, input, ui_input, &running);
                    break;
                case SDL_EVENT_KEY_UP:
                    platform::apply_key_up(ev.key.key, ui_input);
                    break;
                default: break;
            }
        }

        const bool *keys = SDL_GetKeyboardState(nullptr);
        ui_input.confirm_down = ui_input.confirm_down ||
            keys[SDL_SCANCODE_RETURN] || keys[SDL_SCANCODE_SPACE];
        ui_input.cancel_down = ui_input.cancel_down || keys[SDL_SCANCODE_ESCAPE];

        float mx, my;
        Uint32 mb = SDL_GetMouseState(&mx, &my);
        bool pointer_down = (mb & SDL_BUTTON_LMASK) != 0;
        ui_input.pointer_down = pointer_down;
        ui_input.pointer_pressed = pointer_down && !previous_pointer_down;
        ui_input.pointer_released = !pointer_down && previous_pointer_down;
        if (ui_input.pointer_pressed || ui_input.pointer_released) {
            ui_input.source = ::ui::UiFocusSource::Mouse;
        }
        previous_pointer_down = pointer_down;

        control.poll(input, ui_input, running, g_window, ui_pipeline);
        if (control.apply_pointer_override(mx, my, pointer_down)) {
            ui_input.pointer_down = pointer_down;
        }

        int frame_w = 800;
        int frame_h = 500;
        SDL_GetWindowSize(g_window, &frame_w, &frame_h);
        game::ui::GameUiFrame frame = {
            .input = ui_input,
            .layout = { (float)frame_w, (float)frame_h },
            .pointer = { mx, my },
            .demo_input = nullptr,
        };

        ui_pipeline.render_client_ui_frame(frame, [&](Clay_RenderCommandArray &cmds) {
            SDL_SetRenderDrawColor(g_sdl, 12, 14, 22, 255);
            SDL_RenderClear(g_sdl);
            SDL_Clay_RenderClayCommands(&g_clay_rd, &cmds);
            control.capture_after_render(g_sdl, ui_pipeline);
            SDL_RenderPresent(g_sdl);
        });
        control.finish_frame(ui_pipeline);
    }

    control.shutdown();
    react_shutdown();
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
