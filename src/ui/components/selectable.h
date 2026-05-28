#pragma once

#include "../runtime/element.h"

#include <functional>

namespace ui::components {

struct SelectableProps {
  const char *key = nullptr;
  const char *id = nullptr;
  int offset = 0;
  const char *label = nullptr;
  bool selected = false;
  bool disabled = false;
  bool initial_focus = false;
  retained::Length width = retained::Length::points(132.0f);
  retained::Length height = retained::Length::points(34.0f);
  retained::FlexDirection direction = retained::FlexDirection::Column;
  retained::AlignItems align_items = retained::AlignItems::Center;
  retained::JustifyContent justify_content = retained::JustifyContent::Center;
  retained::EdgeSizes padding = {12.0f, 12.0f, 7.0f, 7.0f};
  float gap = 0.0f;
  retained::Color background = {};
  retained::Color border = {};
  retained::Color text_color = {};
  float border_width = 0.0f;
  uint16_t font_size = 0;
  retained::UiChildren children = {};
  std::function<void()> on_focus = {};
  std::function<void()> on_confirm = {};
};

retained::UiElement Selectable(retained::UiElementFrame &frame,
                               const SelectableProps &props);

} // namespace ui::components
