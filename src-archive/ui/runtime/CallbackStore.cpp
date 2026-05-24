#include "ui/runtime/CallbackStore.h"

#include <utility>

namespace ui {

intptr_t CallbackStore::retain(std::function<void()> callback) {
    callbacks_.push_back(std::move(callback));
    return reinterpret_cast<intptr_t>(&callbacks_.back());
}

void CallbackStore::clearAfterPointerUpdate() {
    callbacks_.clear();
}

void CallbackStore::dispatchPress(Clay_ElementId, Clay_PointerData pointer, intptr_t userData) {
    if (pointer.state != CLAY_POINTER_DATA_PRESSED_THIS_FRAME || userData == 0) {
        return;
    }

    auto *callback = reinterpret_cast<std::function<void()> *>(userData);
    if (*callback) {
        (*callback)();
    }
}

}
