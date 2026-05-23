#pragma once

#include <clay.h>

#include <cstdint>
#include <deque>
#include <functional>

namespace ui {

class CallbackStore {
public:
    intptr_t retain(std::function<void()> callback);
    void clearAfterPointerUpdate();

    static void dispatchPress(Clay_ElementId id, Clay_PointerData pointer, intptr_t userData);

private:
    std::deque<std::function<void()>> callbacks_;
};

}
