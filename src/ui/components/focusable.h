#pragma once

#include "../runtime/element.h"

#include <functional>

namespace ui::components {

struct FocusableProps {
  const char *key = nullptr;
  const char *id = nullptr;
  int offset = 0;
  bool disabled = false;
  bool initial_focus = false;
  retained::Length width = retained::Length::auto_size();
  retained::Length height = retained::Length::auto_size();
  float flex_grow = 0.0f;
  retained::FlexDirection direction = retained::FlexDirection::Column;
  retained::AlignItems align_items = retained::AlignItems::Stretch;
  retained::JustifyContent justify_content = retained::JustifyContent::Start;
  retained::EdgeSizes padding = {};
  float gap = 0.0f;
  retained::Color background = {};
  retained::Color border = {};
  float border_width = 0.0f;
  retained::UiChildren children = {};
  std::function<void()> on_focus = {};
  std::function<void()> on_confirm = {};
};

retained::UiElement Focusable(retained::UiElementFrame &frame,
                              const FocusableProps &props);

} // namespace ui::components
