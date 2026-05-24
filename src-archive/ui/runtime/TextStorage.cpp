#include "ui/runtime/TextStorage.h"

namespace ui {

void TextStorage::beginFrame() {
    strings_.clear();
    configs_.clear();
}

Clay_String TextStorage::store(const char *value) {
    auto &stored = strings_.emplace_back(value ? value : "");
    return {.isStaticallyAllocated = false, .length = static_cast<int32_t>(stored.size()), .chars = stored.c_str()};
}

Clay_String TextStorage::store(const std::string &value) {
    auto &stored = strings_.emplace_back(value);
    return {.isStaticallyAllocated = false, .length = static_cast<int32_t>(stored.size()), .chars = stored.c_str()};
}

Clay_TextElementConfig *TextStorage::config(Clay_Color color, uint16_t size, uint16_t font) {
    auto &stored = configs_.emplace_back(Clay_TextElementConfig{
        .textColor = color,
        .fontId = font,
        .fontSize = size,
        .wrapMode = CLAY_TEXT_WRAP_WORDS
    });
    return &stored;
}

}
