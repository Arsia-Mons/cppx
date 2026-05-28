#pragma once

#include "../retained/element.h"

namespace ui::components {

struct TextProps {
  const char *key = nullptr;
  const char *value = nullptr;
  retained::Length width = retained::Length::auto_size();
  retained::Length height = retained::Length::auto_size();
  retained::Color text_color = {};
  uint16_t font_size = 0;
};

retained::UiElement Text(retained::UiElementFrame &frame,
                         const TextProps &props);

} // namespace ui::components
