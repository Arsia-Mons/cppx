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

#include "game/ui/game_ui_pipeline.h"
#include "platform/control_mailbox.h"
#include "platform/sdl/input.h"
#include "platform/sdl/window.h"
#include "react.h"
#include "renderer/font_registry.h"
#include "renderer/sdl_clay_renderer.h"
#include "shooter/shooter_game.h"
#include "shooter/shooter_ui.h"

#include <curl/curl.h>

#include <memory>
#include <sstream>
#include <stdio.h>
#include <string.h>
#include <string>

// ----------------------------------------------------------------------------
// Local SDL/Clay/font state.
// ----------------------------------------------------------------------------

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

    platform::sdl::Window window;
    if (!window.initialize("clay + react hello-world", 800, 500,
                           /*vsync=*/control_dir == nullptr)) {
        return 1;
    }
    SDL_Window   *g_window   = window.handle();
    SDL_Renderer *g_renderer = window.renderer();

    renderer::FontRegistry fonts;
    if (!fonts.initialize(g_renderer)) {
        return 1;
    }
    renderer::SdlClayRenderer clay_renderer;
    if (!clay_renderer.initialize(g_renderer, fonts)) {
        return 1;
    }

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
    Clay_SetMeasureTextFunction(renderer::FontRegistry::measure_thunk, &fonts);

    react_init(clay_ctx);

    game::ui::GameUiPipeline ui_pipeline;
    shooter::ShooterGame shooter_game;
    bool running = true;
    ui_pipeline.client_ui().push_screen(
        std::make_unique<shooter::MainMenuScreen>(&shooter_game, [&running] {
            running = false;
        }));
    platform::ControlMailbox control;
    control.set_game_state_json_provider([&shooter_game] {
        return shooter_state_json(shooter_game);
    });
    if (control_dir && !control.init(control_dir)) {
        return 1;
    }

    // --- main loop ---
    bool previous_pointer_down = false;
    while (running) {
        ::ui::UiInputFrame ui_input = {};

        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            switch (ev.type) {
                case SDL_EVENT_QUIT:
                    running = false; break;
                case SDL_EVENT_KEY_DOWN:
                    if (ev.key.repeat) break;
                    platform::apply_key_down(ev.key.key, ui_input, &running);
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

        control.poll(ui_input, running, g_window, ui_pipeline);
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
        };

        ui_pipeline.render_client_ui_frame(frame, [&](Clay_RenderCommandArray &cmds) {
            clay_renderer.clear({ 12, 14, 22, 255 });
            clay_renderer.render(cmds);
            control.capture_after_render(g_renderer, ui_pipeline);
            clay_renderer.present();
        });
        control.finish_frame(ui_pipeline);
    }

    control.shutdown();
    react_shutdown();
    SDL_free(clay_buf);
    window.shutdown();
    TTF_Quit();
    SDL_Quit();
    curl_global_cleanup();
    return 0;
}
