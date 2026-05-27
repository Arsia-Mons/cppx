#include "selectable.h"

#include "../../react.h"
#include "clay_text.h"
#include "focusable.h"
#include "visual_state.h"

namespace ui {

void Selectable(const SelectableProps &props) {
    REACT_COMPONENT_BEGIN_KEY("Selectable", props.id.id) {
        Focusable({
            .id = props.id,
            .disabled = props.disabled,
            .nav = props.nav,
            .on_confirm = props.on_select,
        }, [&](const UiFocusableState &focus) {
            VisualState visual = derive_visual_state(focus, {
                .selected = props.selected,
                .disabled = props.disabled,
            });
            uint16_t border_width = visual.targeted ? 2 : 1;

            CLAY({
                .id = focus.id,
                .layout = {
                    .sizing = { CLAY_SIZING_FIXED(132), CLAY_SIZING_FIXED(34) },
                    .padding = { 12, 12, 7, 7 },
                    .childAlignment = { CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER },
                },
                .backgroundColor = visual.chosen
                    ? Clay_Color{ 42, 80, 60, 255 }
                    : Clay_Color{ 24, 28, 36, 255 },
                .cornerRadius = CLAY_CORNER_RADIUS(4),
                .border = {
                    .color = visual.targeted
                        ? Clay_Color{ 122, 176, 238, 255 }
                        : Clay_Color{ 78, 88, 104, 255 },
                    .width = CLAY_BORDER_OUTSIDE(border_width),
                },
            }) {
                CLAY_TEXT(clay_text(props.label),
                    CLAY_TEXT_CONFIG({
                        .textColor = props.disabled
                            ? Clay_Color{ 126, 134, 148, 255 }
                            : Clay_Color{ 226, 234, 242, 255 },
                        .fontSize = 14,
                    }));
            }
        });
    } REACT_COMPONENT_END();
}

} // namespace ui
