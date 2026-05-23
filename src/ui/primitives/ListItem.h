#pragma once

#include "ui/primitives/Button.h"

namespace ui {

void ListItem(UiFrameContext &frame, Clay_ElementId id, const std::string &label, bool selected, std::function<void()> onPressed);

}
