#pragma once

#include "../retained/element.h"

#include <functional>

namespace ui::components {

struct ToggleProps {
  const char *key = nullptr;
  const char *id = nullptr;
  int offset = 0;
  const char *label = nullptr;
  bool checked = false;
  bool disabled = false;
  bool initial_focus = false;
  retained::Length width = retained::Length::points(178.0f);
  retained::Length height = retained::Length::points(38.0f);
  std::function<void()> on_focus = {};
  std::function<void(bool)> on_change = {};
};

retained::UiElement Toggle(retained::UiElementFrame &frame,
                           const ToggleProps &props);

} // namespace ui::components
