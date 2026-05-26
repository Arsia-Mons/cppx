#include "button.h"

#include "../../react.h"
#include "clay_text.h"
#include "focusable.h"
#include "visual_state.h"

namespace ui {

struct ButtonStyle {
    Clay_Color background;
    Clay_Color border;
    Clay_Color text;
    uint16_t border_width;
};

static ButtonStyle button_style(VisualState visual) {
    if (visual.unavailable) {
        return {
            .background = { 30, 34, 42, 255 },
            .border = { 62, 68, 78, 255 },
            .text = { 126, 134, 148, 255 },
            .border_width = 1,
        };
    }
    if (visual.active) {
        return {
            .background = { 55, 98, 150, 255 },
            .border = { 145, 198, 255, 255 },
            .text = { 248, 252, 255, 255 },
            .border_width = 2,
        };
    }
    if (visual.targeted) {
        return {
            .background = { 42, 72, 108, 255 },
            .border = { 122, 176, 238, 255 },
            .text = { 244, 248, 252, 255 },
            .border_width = 2,
        };
    }
    return {
        .background = { 24, 28, 36, 255 },
        .border = { 78, 88, 104, 255 },
        .text = { 226, 234, 242, 255 },
        .border_width = 1,
    };
}

void Button(const ButtonProps &props) {
    REACT_COMPONENT_BEGIN_KEY("Button", props.id.id) {
        Focusable({
            .id = props.id,
            .disabled = props.disabled,
            .nav = props.nav,
            .on_confirm = props.on_confirm,
        }, [&](const UiFocusableState &focus) {
            VisualState visual = derive_visual_state(focus, {
                .disabled = props.disabled,
            });
            ButtonStyle style = button_style(visual);

            CLAY({
                .id = focus.id,
                .layout = {
                    .sizing = { CLAY_SIZING_FIXED(132), CLAY_SIZING_FIXED(38) },
                    .padding = { 14, 14, 8, 8 },
                    .childAlignment = { CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER },
                },
                .backgroundColor = style.background,
                .border = {
                    .width = CLAY_BORDER_OUTSIDE(style.border_width),
                    .color = style.border,
                },
                .cornerRadius = CLAY_CORNER_RADIUS(4),
            }) {
                CLAY_TEXT(clay_text(props.label),
                    CLAY_TEXT_CONFIG({
                        .textColor = style.text,
                        .fontSize = 15,
                    }));
            }
        });
    } REACT_COMPONENT_END();
}

} // namespace ui
