#include "ui/primitives/Field.h"

#include "ui/design/Theme.h"
#include "ui/primitives/Text.h"

#include <utility>

namespace ui {

void Field(
    UiFrameContext &frame,
    Clay_ElementId id,
    const std::string &label,
    const std::string &value,
    bool focused,
    bool password,
    std::function<void()> onFocus
) {
    const intptr_t payload = frame.callbacks.retain(std::move(onFocus));
    std::string display = password ? std::string(value.size(), '*') : value;
    if (display.empty()) {
        display = focused ? "|" : "";
    } else if (focused) {
        display += "|";
    }

    CLAY({ .id = id,
        .layout = {
            .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIXED(78)},
            .padding = CLAY_PADDING_ALL(10),
            .childGap = 8,
            .layoutDirection = CLAY_TOP_TO_BOTTOM
        },
        .backgroundColor = focused ? color::SurfaceRaised : color::SurfaceMuted,
        .cornerRadius = radius::Small,
        .border = {.color = focused ? color::Accent : color::Border, .width = CLAY_BORDER_ALL(1)}
    }) {
        Clay_OnHover(CallbackStore::dispatchPress, payload);
        Text(frame, label, {.color = color::TextMuted, .size = type::Caption, .font = font::Body});
        Text(frame, display, {.color = color::Text, .size = 19, .font = font::Mono});
    }
}

}
