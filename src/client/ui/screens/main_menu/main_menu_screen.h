#pragma once

#include <functional>

#include "../../navigation/ui_screen.h"
#include "../../../../game/shooter_game.h"

namespace shooter {

class MainMenuScreen final : public client::ui::UiScreen {
public:
    explicit MainMenuScreen(ShooterGame *game, std::function<void()> request_quit = {})
        : game_(game), request_quit_(request_quit) {}

    const char *debug_name() const override { return "MainMenu"; }
    void build_ui() override;

private:
    ShooterGame          *game_         = nullptr;
    std::function<void()> request_quit_ = {};
};

std::function<void()> use_exit_to_main_menu();

} // namespace shooter
