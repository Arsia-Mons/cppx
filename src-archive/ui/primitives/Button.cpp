#include "ui/primitives/Button.h"

#include "ui/design/Theme.h"
#include "ui/primitives/Text.h"

#include <utility>

namespace ui {
namespace {

Clay_Color hoverColor(Clay_Color normal, Clay_Color hover) {
    return Clay_Hovered() ? hover : normal;
}

}

void Button(
    UiFrameContext &frame,
    Clay_ElementId id,
    const std::string &label,
    ButtonVariant variant,
    std::function<void()> onPressed
) {
    Clay_Color bg = color::SurfaceRaised;
    Clay_Color fg = color::Text;
    Clay_Color hover = {54, 60, 70, 255};
    Clay_Color border = color::Border;

    if (variant == ButtonVariant::Primary) {
        bg = color::Accent;
        fg = color::AccentText;
        hover = color::AccentHover;
        border = color::AccentHover;
    } else if (variant == ButtonVariant::Ghost) {
        bg = {0, 0, 0, 0};
        hover = {45, 50, 58, 255};
    } else if (variant == ButtonVariant::Danger) {
        bg = color::Danger;
        hover = {238, 116, 116, 255};
        fg = {28, 5, 5, 255};
        border = hover;
    }

    const intptr_t payload = frame.callbacks.retain(std::move(onPressed));
    CLAY({ .id = id,
        .layout = {
            .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIXED(44)},
            .padding = {.left = 14, .right = 14, .top = 0, .bottom = 0},
            .childAlignment = {.x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER}
        },
        .backgroundColor = hoverColor(bg, hover),
        .cornerRadius = radius::Small,
        .border = {.color = border, .width = CLAY_BORDER_ALL(1)}
    }) {
        Clay_OnHover(CallbackStore::dispatchPress, payload);
        Text(frame, label, {.color = fg, .size = 17, .font = font::Body});
    }
}

}
