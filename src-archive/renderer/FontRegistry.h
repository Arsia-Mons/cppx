#pragma once

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <clay.h>

#include <array>

namespace renderer {

class FontRegistry {
public:
    FontRegistry() = default;
    ~FontRegistry();

    FontRegistry(const FontRegistry &) = delete;
    FontRegistry &operator=(const FontRegistry &) = delete;

    bool initialize(SDL_Renderer *renderer);
    Clay_Dimensions measureText(Clay_StringSlice text, Clay_TextElementConfig *config);

    TTF_TextEngine *textEngine() const { return textEngine_; }
    TTF_Font **fonts() { return fontPointers_.data(); }

private:
    TTF_TextEngine *textEngine_ = nullptr;
    std::array<TTF_Font *, 3> fonts_{};
    std::array<TTF_Font *, 3> fontPointers_{};
};

}
