#pragma once

#include <functional>

#include "client/ui/navigation/ui_screen.h"

namespace shooter {

class MainMenuScreen final : public client::ui::UiScreen {
public:
  MainMenuScreen() = default;

  const char *debug_name() const override { return "MainMenu"; }
  bool build_element(::ui::UiElementFrame &frame,
                     ::ui::UiElement *out) override;
  void build_ui() override;
};

} // namespace shooter
