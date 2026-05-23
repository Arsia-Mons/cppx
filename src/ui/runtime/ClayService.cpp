#include "ui/runtime/ClayService.h"

#include <cstdio>

namespace {

void handleClayError(Clay_ErrorData error) {
    std::fprintf(stderr, "Clay error: %.*s\n", error.errorText.length, error.errorText.chars);
}

}

namespace ui {

ClayService::ClayService(float width, float height) {
    const uint32_t memorySize = Clay_MinMemorySize();
    memory_.resize(memorySize);
    Clay_Arena arena = Clay_CreateArenaWithCapacityAndMemory(memorySize, memory_.data());
    context_ = Clay_Initialize(arena, {.width = width, .height = height}, {.errorHandlerFunction = handleClayError});
}

void ClayService::beginFrame(const UiInputState &input, UiFrameContext &frame) {
    frame.text.beginFrame();
    Clay_SetCurrentContext(context_);
    Clay_SetLayoutDimensions({.width = input.width, .height = input.height});
    Clay_SetPointerState({.x = input.mouseX, .y = input.mouseY}, input.pointerDown);
    Clay_UpdateScrollContainers(true, input.scrollDelta, input.deltaSeconds);
    frame.callbacks.clearAfterPointerUpdate();
    Clay_BeginLayout();
}

Clay_RenderCommandArray ClayService::endFrame() {
    return Clay_EndLayout();
}

}
