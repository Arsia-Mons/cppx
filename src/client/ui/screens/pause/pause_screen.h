#pragma once

#include <functional>

#include "../../navigation/ui_screen.h"

namespace shooter {

class PauseScreen final : public client::ui::OverlayScreen {
public:
  PauseScreen() = default;

  const char *debug_name() const override { return "Pause"; }
  bool build_element(::ui::retained::UiElementFrame &frame,
                     ::ui::retained::UiElement *out) override;
  void build_ui() override;
};

std::function<void()> use_push_pause_screen();

} // namespace shooter
