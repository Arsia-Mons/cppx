#include "toggle.h"

#include "../../react.h"
#include "clay_text.h"
#include "focusable.h"
#include "visual_state.h"

namespace ui {

static Clay_Color toggle_background(VisualState visual) {
    if (visual.unavailable) return { 30, 32, 38, 255 };
    if (visual.active) return { 44, 92, 128, 255 };
    if (visual.targeted) return { 35, 62, 88, 255 };
    return { 24, 28, 36, 255 };
}

void Toggle(const ToggleProps &props) {
    REACT_COMPONENT_BEGIN_KEY("Toggle", props.id.id) {
        Focusable({
            .id = props.id,
            .disabled = props.disabled,
            .nav = props.nav,
            .on_confirm = [props] {
                if (props.on_change) {
                    props.on_change(!props.checked);
                }
            },
        }, [&](const UiFocusableState &focus) {
            VisualState visual = derive_visual_state(focus, {
                .checked = props.checked,
                .disabled = props.disabled,
            });
            uint16_t border_width = visual.targeted ? 2 : 1;

            CLAY({
                .id = focus.id,
                .layout = {
                    .sizing = { CLAY_SIZING_FIXED(178), CLAY_SIZING_FIXED(38) },
                    .padding = { 10, 10, 8, 8 },
                    .childGap = 10,
                    .childAlignment = { CLAY_ALIGN_X_LEFT, CLAY_ALIGN_Y_CENTER },
                    .layoutDirection = CLAY_LEFT_TO_RIGHT,
                },
                .backgroundColor = toggle_background(visual),
                .cornerRadius = CLAY_CORNER_RADIUS(4),
                .border = {
                    .color = visual.chosen
                        ? Clay_Color{ 136, 210, 148, 255 }
                        : Clay_Color{ 78, 88, 104, 255 },
                    .width = CLAY_BORDER_OUTSIDE(border_width),
                },
            }) {
                CLAY({
                    .id = CLAY_ID_LOCAL("ToggleMark"),
                    .layout = {
                        .sizing = { CLAY_SIZING_FIXED(18), CLAY_SIZING_FIXED(18) },
                    },
                    .backgroundColor = visual.chosen
                        ? Clay_Color{ 136, 210, 148, 255 }
                        : Clay_Color{ 56, 62, 72, 255 },
                    .cornerRadius = CLAY_CORNER_RADIUS(3),
                }) {}
                CLAY_TEXT(clay_text(props.label),
                    CLAY_TEXT_CONFIG({
                        .textColor = props.disabled
                            ? Clay_Color{ 126, 134, 148, 255 }
                            : Clay_Color{ 226, 234, 242, 255 },
                        .fontSize = 15,
                    }));
            }
        });
    } REACT_COMPONENT_END();
}

} // namespace ui
