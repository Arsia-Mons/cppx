#pragma once

#include <string.h>

#include <clay.h>

namespace ui {

inline Clay_String clay_text(const char *text) {
    return Clay_String{ false, text ? (int32_t)strlen(text) : 0, text ? text : "" };
}

} // namespace ui
