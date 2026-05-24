#pragma once

#include "ui/primitives/Button.h"

namespace ui {

void Tab(UiFrameContext &frame, Clay_ElementId id, const std::string &label, bool selected, std::function<void()> onPressed);

}
