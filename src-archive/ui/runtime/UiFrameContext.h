#pragma once

#include "ui/runtime/CallbackStore.h"
#include "ui/runtime/TextStorage.h"

namespace ui {

struct UiFrameContext {
    CallbackStore &callbacks;
    TextStorage &text;
};

}
