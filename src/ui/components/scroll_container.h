#pragma once

#include "../runtime/element.h"

namespace ui::components {

struct ScrollContainerProps {
  const char *key = nullptr;
  const char *id = nullptr;
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
  retained::Color text_color = {};
  float border_width = 0.0f;
  uint16_t font_size = 0;
  retained::UiChildren children = {};
};

retained::UiElement ScrollContainer(retained::UiElementFrame &frame,
                                    const ScrollContainerProps &props);

} // namespace ui::components
