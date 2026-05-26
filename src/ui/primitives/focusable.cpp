#include "focusable.h"

namespace ui {

void Focusable(const FocusableProps &props, FocusableRender render) {
    UiFocusableState state = ui_focusable({
        .id = props.id,
        .disabled = props.disabled,
        .nav = props.nav,
        .on_confirm = props.on_confirm,
        .on_focus = props.on_focus,
    });
    render(state);
}

} // namespace ui
