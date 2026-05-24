#pragma once

#include "ui/design/Theme.h"
#include "ui/runtime/UiFrameContext.h"

#include <string>

namespace ui {

struct TextStyle {
    Clay_Color color = color::Text;
    uint16_t size = type::Body;
    uint16_t font = font::Body;
};

void Text(UiFrameContext &frame, const std::string &value, TextStyle style = {});
void Heading(UiFrameContext &frame, const std::string &value);

}
