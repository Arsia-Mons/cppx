#include "app.h"

#include "game_loop.h"
#include "../react.h"
#include "../renderer/text_measure_impl.h"
#include "../client/ui/app_shell/app_shell_provider.h"
#include "../client/ui/providers/shooter_provider.h"
#include "client/ui/screens/main_menu/main_menu_screen.h"

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <curl/curl.h>

#include <memory>
#include <sstream>
#include <stdio.h>
#include <string>

namespace app {

static std::string json_escape(const char *value) {
    std::string out;
    if (!value) return out;
    for (const char *p = value; *p; ++p) {
        switch (*p) {
            case '\\': out += "\\\\"; break;
            case '"':  out += "\\\""; break;
            case '\n': out += "\\n";  break;
            case '\r': out += "\\r";  break;
            case '\t': out += "\\t";  break;
            default:   out += *p;     break;
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
        << "\"selected_weapon\":" << game.selected_weapon()
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

App::App()  = default;
App::~App() { shutdown(); }

bool App::initialize(const AppOptions &options) {
    options_ = options;

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
        return false;
    }
    if (!TTF_Init()) {
        fprintf(stderr, "TTF_Init: %s\n", SDL_GetError());
        return false;
    }
    curl_global_init(CURL_GLOBAL_DEFAULT);

    if (!window_.initialize(options.title, options.width, options.height,
                            /*vsync=*/options.control_dir == nullptr)) {
        return false;
    }
    if (!fonts_.initialize(window_.renderer())) {
        return false;
    }
    // Install the ONE SDL_ttf-backed text measurer (design §10.1). It is used by
    // BOTH the Yoga measure shim and the draw-list transcriber, so layout and
    // paint measure identically. ui/ stays SDL-free; this is the only seam.
    renderer::install_text_measurer(&fonts_);
    if (!surface_.initialize(window_.renderer(), fonts_)) {
        return false;
    }

    react_init_runtime();

    ui_pipeline_.set_frame_provider([this](::ui::UiElement child) {
        shooter::ShooterContextValue        game_ctx { .game = &shooter_game_ };
        client::ui::AppShellContextValue    shell_ctx { .request_quit = [this] { running_ = false; } };
        return shooter::ShooterProvider(
            game_ctx,
            ::ui::children({
                client::ui::AppShellProvider(shell_ctx, ::ui::children({child})),
            }));
    });

    ui_pipeline_.client_ui().push_screen(std::make_unique<shooter::MainMenuScreen>());

    control_.set_game_state_json_provider([this] {
        return shooter_state_json(shooter_game_);
    });
    if (options.control_dir && !control_.init(options.control_dir)) {
        return false;
    }

    initialized_ = true;
    return true;
}

int App::run() {
    if (!initialized_) return 1;
    GameLoop loop(window_, surface_, ui_pipeline_, control_, running_);
    while (running_) {
        loop.tick();
    }
    return 0;
}

void App::shutdown() {
    if (!initialized_) return;
    control_.shutdown();
    react_shutdown();
    surface_.shutdown(); // free the supersample target before the renderer dies
    fonts_.shutdown();
    window_.shutdown();
    TTF_Quit();
    SDL_Quit();
    curl_global_cleanup();
    initialized_ = false;
}

} // namespace app
