#include "ui/primitives/Text.h"

namespace ui {

void Text(UiFrameContext &frame, const std::string &value, TextStyle style) {
    CLAY_TEXT(frame.text.store(value), frame.text.config(style.color, style.size, style.font));
}

void Heading(UiFrameContext &frame, const std::string &value) {
    Text(frame, value, {.color = color::Text, .size = type::Heading, .font = font::Title});
}

}
