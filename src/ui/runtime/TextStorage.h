#pragma once

#include <clay.h>

#include <deque>
#include <string>

namespace ui {

class TextStorage {
public:
    void beginFrame();

    Clay_String store(const char *value);
    Clay_String store(const std::string &value);
    Clay_TextElementConfig *config(Clay_Color color, uint16_t size, uint16_t font);

private:
    std::deque<std::string> strings_;
    std::deque<Clay_TextElementConfig> configs_;
};

}
