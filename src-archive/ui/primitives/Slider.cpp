#include "ui/primitives/Slider.h"

#include "ui/primitives/ProgressBar.h"

namespace ui {

void Slider(UiFrameContext &frame, Clay_ElementId id, const std::string &label, float value, Clay_Color fill) {
    ProgressBar(frame, id, label, value, fill);
}

}
