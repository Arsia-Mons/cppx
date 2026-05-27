#pragma once

#include <functional>

#include "../../ui_screen.h"
#include "../../../../game/shooter_game.h"

namespace shooter {

class PauseScreen final : public client::ui::UiScreen {
public:
    explicit PauseScreen(ShooterGame *game, std::function<void()> request_quit = {})
        : game_(game), request_quit_(request_quit) {}

    const char *debug_name() const override { return "Pause"; }
    bool is_overlay() const override { return true; }
    void build_ui() override;

private:
    ShooterGame          *game_         = nullptr;
    std::function<void()> request_quit_ = {};
};

std::function<void()> use_push_pause_screen();

} // namespace shooter
