#pragma once

#include <functional>

#include "../../navigation/ui_screen.h"

namespace shooter {

class MainMenuScreen final : public client::ui::UiScreen {
public:
    MainMenuScreen() = default;

    const char *debug_name() const override { return "MainMenu"; }
    void build_ui() override;
};

std::function<void()> use_exit_to_main_menu();

} // namespace shooter
