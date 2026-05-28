#pragma once

#include <functional>

#include "../../navigation/ui_screen.h"

namespace shooter {

class OptionsScreen final : public client::ui::OverlayScreen {
public:
  OptionsScreen() = default;

  const char *debug_name() const override { return "Options"; }
  bool build_element(::ui::retained::UiElementFrame &frame,
                     ::ui::retained::UiElement *out) override;
  void build_ui() override;
};

std::function<void()> use_push_options_screen();

} // namespace shooter
