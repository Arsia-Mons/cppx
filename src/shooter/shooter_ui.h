#pragma once

#include "../client/ui/ui_screen.h"
#include "shooter_game.h"

#include <functional>

namespace shooter {

class MainMenuScreen final : public client::ui::UiScreen {
public:
    explicit MainMenuScreen(ShooterGame *game, std::function<void()> request_quit = {})
        : game_(game), request_quit_(request_quit) {}

    const char *debug_name() const override { return "MainMenu"; }
    void build_ui() override;

private:
    ShooterGame *game_ = nullptr;
    std::function<void()> request_quit_ = {};
};

class ShooterGameScreen final : public client::ui::UiScreen {
public:
    explicit ShooterGameScreen(ShooterGame *game, std::function<void()> request_quit = {})
        : game_(game), request_quit_(request_quit) {}

    const char *debug_name() const override { return "ShooterGame"; }
    void build_ui() override;

private:
    ShooterGame *game_ = nullptr;
    std::function<void()> request_quit_ = {};
};

class PauseScreen final : public client::ui::UiScreen {
public:
    explicit PauseScreen(ShooterGame *game, std::function<void()> request_quit = {})
        : game_(game), request_quit_(request_quit) {}

    const char *debug_name() const override { return "Pause"; }
    bool is_overlay() const override { return true; }
    void build_ui() override;

private:
    ShooterGame *game_ = nullptr;
    std::function<void()> request_quit_ = {};
};

class LoadoutScreen final : public client::ui::UiScreen {
public:
    explicit LoadoutScreen(ShooterGame *game, std::function<void()> request_quit = {})
        : game_(game), request_quit_(request_quit) {}

    const char *debug_name() const override { return "Loadout"; }
    bool is_overlay() const override { return true; }
    void build_ui() override;

private:
    ShooterGame *game_ = nullptr;
    std::function<void()> request_quit_ = {};
};

class OptionsScreen final : public client::ui::UiScreen {
public:
    explicit OptionsScreen(ShooterGame *game, std::function<void()> request_quit = {})
        : game_(game), request_quit_(request_quit) {}

    const char *debug_name() const override { return "Options"; }
    bool is_overlay() const override { return true; }
    void build_ui() override;

private:
    ShooterGame *game_ = nullptr;
    std::function<void()> request_quit_ = {};
};

} // namespace shooter
