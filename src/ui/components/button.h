#pragma once

#include "../retained/element.h"

#include <functional>

namespace ui::components {

struct ButtonProps {
  const char *key = nullptr;
  const char *id = nullptr;
  int offset = 0;
  const char *label = nullptr;
  bool disabled = false;
  bool initial_focus = false;
  retained::Length width = retained::Length::points(132.0f);
  retained::Length height = retained::Length::points(38.0f);
  std::function<void()> on_focus = {};
  std::function<void()> on_confirm = {};
};

retained::UiElement Button(retained::UiElementFrame &frame,
                           const ButtonProps &props);

} // namespace ui::components
