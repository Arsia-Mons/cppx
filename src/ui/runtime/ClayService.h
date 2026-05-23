#pragma once

#include "ui/runtime/UiFrameContext.h"
#include "ui/runtime/UiInputState.h"

#include <clay.h>

#include <cstdint>
#include <memory>
#include <vector>

namespace ui {

class ClayService {
public:
    ClayService(float width, float height);

    void beginFrame(const UiInputState &input, UiFrameContext &frame);
    Clay_RenderCommandArray endFrame();

private:
    std::vector<std::byte> memory_;
    Clay_Context *context_ = nullptr;
};

}
