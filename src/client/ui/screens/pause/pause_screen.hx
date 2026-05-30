#pragma once

#include <functional>

#include "client/ui/navigation/ui_screen.h"

namespace shooter {

class PauseScreen final : public client::ui::OverlayScreen {
public:
  PauseScreen() = default;

  const char *debug_name() const override { return "Pause"; }
  bool build_element(::ui::UiElementFrame &frame,
                     ::ui::UiElement *out) override;
  void build_ui() override;
};

} // namespace shooter
