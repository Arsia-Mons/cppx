#pragma once

#include <functional>

#include "client/ui/app_shell/navigation/ui_screen.h"

namespace shooter {

class ShooterGameScreen final : public client::ui::UiScreen {
public:
  ShooterGameScreen() = default;

  const char *debug_name() const override { return "ShooterGame"; }
  bool build_element(::ui::UiElementFrame &frame,
                     ::ui::UiElement *out) override;
  void build_ui() override;
};

} // namespace shooter
