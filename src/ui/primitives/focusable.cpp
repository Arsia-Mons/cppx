#include "focusable.h"

#include "../../react.h"

namespace ui {

void Focusable(const FocusableProps &props, FocusableRender render) {
    REACT_FRAGMENT_COMPONENT_BEGIN_KEY("Focusable", props.id.id) {
        UiFocusableState state = ui_focusable({
            .id = props.id,
            .disabled = props.disabled,
            .nav = props.nav,
            .on_confirm = props.on_confirm,
            .on_focus = props.on_focus,
        });
        if (render) {
            render(state);
        }
    } REACT_FRAGMENT_COMPONENT_END();
}

} // namespace ui
