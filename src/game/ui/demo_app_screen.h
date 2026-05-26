#pragma once

#include "../../client/ui/ui_screen.h"

namespace game::ui {

class DemoAppScreen final : public client::ui::UiScreen {
public:
    const char *debug_name() const override { return "DemoApp"; }
    void build_ui() override;
};

} // namespace game::ui
