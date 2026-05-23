#pragma once

#include "ui/runtime/UiFrameContext.h"

#include <clay.h>

#include <string>

namespace ui {

void ProgressBar(UiFrameContext &frame, Clay_ElementId id, const std::string &label, float value, Clay_Color fill);

}
