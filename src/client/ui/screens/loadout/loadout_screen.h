#pragma once

#include <functional>

#include "../../navigation/ui_screen.h"
#include "../../../../game/shooter_game.h"

namespace shooter {

class LoadoutScreen final : public client::ui::UiScreen {
public:
    explicit LoadoutScreen(ShooterGame *game, std::function<void()> request_quit = {})
        : game_(game), request_quit_(request_quit) {}

    const char *debug_name() const override { return "Loadout"; }
    bool is_overlay() const override { return true; }
    void build_ui() override;

    bool compare_enabled() const     { return compare_enabled_; }
    void set_compare_enabled(bool v) { compare_enabled_ = v; }

private:
    ShooterGame          *game_            = nullptr;
    std::function<void()> request_quit_    = {};
    bool                  compare_enabled_ = false;
};

std::function<void()> use_push_loadout_screen();

} // namespace shooter
