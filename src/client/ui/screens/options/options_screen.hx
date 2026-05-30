#pragma once

#include "client/ui/navigation/ui_screen.h"

namespace shooter {

class OptionsScreen final : public client::ui::OverlayScreen {
public:
  OptionsScreen() = default;

  const char *debug_name() const override { return "Options"; }
  bool build_element(::ui::UiElementFrame &frame,
                     ::ui::UiElement *out) override;
  void build_ui() override;
};

} // namespace shooter
