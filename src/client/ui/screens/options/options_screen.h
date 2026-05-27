#pragma once

#include <functional>

#include "../../navigation/ui_screen.h"
#include "../../../../game/shooter_game.h"

namespace shooter {

class OptionsScreen final : public client::ui::UiScreen {
public:
    explicit OptionsScreen(ShooterGame *game, std::function<void()> request_quit = {})
        : game_(game), request_quit_(request_quit) {}

    const char *debug_name() const override { return "Options"; }
    bool is_overlay() const override { return true; }
    void build_ui() override;

private:
    ShooterGame          *game_         = nullptr;
    std::function<void()> request_quit_ = {};
};

std::function<void()> use_push_options_screen();

} // namespace shooter
