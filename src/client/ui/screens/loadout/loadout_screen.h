#pragma once

#include "../../app_shell/navigation/ui_screen.h"

namespace shooter {

class LoadoutScreen final : public client::ui::OverlayScreen {
public:
  LoadoutScreen() = default;

  const char *debug_name() const override { return "Loadout"; }
  bool build_element(::ui::UiElementFrame &frame,
                     ::ui::UiElement *out) override;
  void build_ui() override;
};

} // namespace shooter
