#include "ui/primitives/Tabs.h"

namespace ui {

void Tab(UiFrameContext &frame, Clay_ElementId id, const std::string &label, bool selected, std::function<void()> onPressed) {
    Button(frame, id, label, selected ? ButtonVariant::Primary : ButtonVariant::Secondary, std::move(onPressed));
}

}
