#pragma once

#include "../client/ui/ui_screen.h"
#include "shooter_game.h"

namespace shooter {

class ShooterGameScreen final : public client::ui::UiScreen {
public:
    explicit ShooterGameScreen(ShooterGame *game) : game_(game) {}

    const char *debug_name() const override { return "ShooterGame"; }
    void build_ui() override;

private:
    ShooterGame *game_ = nullptr;
};

class LoadoutScreen final : public client::ui::UiScreen {
public:
    explicit LoadoutScreen(ShooterGame *game) : game_(game) {}

    const char *debug_name() const override { return "Loadout"; }
    bool is_overlay() const override { return true; }
    void build_ui() override;

private:
    ShooterGame *game_ = nullptr;
};

class OptionsScreen final : public client::ui::UiScreen {
public:
    explicit OptionsScreen(ShooterGame *game) : game_(game) {}

    const char *debug_name() const override { return "Options"; }
    bool is_overlay() const override { return true; }
    void build_ui() override;

private:
    ShooterGame *game_ = nullptr;
};

} // namespace shooter
