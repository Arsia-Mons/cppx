#pragma once

#include <functional>

#include "../../ui_screen.h"
#include "../../../../game/shooter_game.h"

namespace shooter {

class ShooterGameScreen final : public client::ui::UiScreen {
public:
    explicit ShooterGameScreen(ShooterGame *game, std::function<void()> request_quit = {})
        : game_(game), request_quit_(request_quit) {}

    const char *debug_name() const override { return "ShooterGame"; }
    void build_ui() override;

private:
    ShooterGame          *game_         = nullptr;
    std::function<void()> request_quit_ = {};
};

std::function<void()> use_start_match();

} // namespace shooter
