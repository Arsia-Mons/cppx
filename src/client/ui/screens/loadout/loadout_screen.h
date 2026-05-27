#pragma once

#include <functional>

#include "../../navigation/ui_screen.h"

namespace shooter {

class LoadoutScreen final : public client::ui::OverlayScreen {
public:
    LoadoutScreen() = default;

    const char *debug_name() const override { return "Loadout"; }
    void build_ui() override;
};

std::function<void()> use_push_loadout_screen();

} // namespace shooter
