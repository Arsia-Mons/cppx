#pragma once

#include <clay.h>

namespace ui {

struct UiInputState {
    float width = 1280.0f;
    float height = 720.0f;
    float mouseX = 0.0f;
    float mouseY = 0.0f;
    bool pointerDown = false;
    Clay_Vector2 scrollDelta{0.0f, 0.0f};
    float deltaSeconds = 1.0f / 60.0f;
};

}
