#pragma once

#include <functional>

#include "../../navigation/ui_screen.h"

namespace shooter {

class ShooterGameScreen final : public client::ui::UiScreen {
public:
  ShooterGameScreen() = default;

  const char *debug_name() const override { return "ShooterGame"; }
  bool build_element(::ui::retained::UiElementFrame &frame,
                     ::ui::retained::UiElement *out) override;
  void build_ui() override;
};

std::function<void()> use_start_match();

} // namespace shooter
