#include "ui/primitives/ListItem.h"

namespace ui {

void ListItem(UiFrameContext &frame, Clay_ElementId id, const std::string &label, bool selected, std::function<void()> onPressed) {
    Button(frame, id, label, selected ? ButtonVariant::Primary : ButtonVariant::Ghost, std::move(onPressed));
}

}
