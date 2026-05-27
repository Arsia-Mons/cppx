#pragma once

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

namespace renderer {

class FontRegistry {
public:
    FontRegistry() = default;
    ~FontRegistry();

    FontRegistry(const FontRegistry &) = delete;
    FontRegistry &operator=(const FontRegistry &) = delete;

    bool initialize(SDL_Renderer *renderer, float default_size = 16.0f);
    void shutdown();

    TTF_TextEngine *text_engine() const { return text_engine_; }
    TTF_Font       *default_font() const { return default_font_; }

private:
    static TTF_Font *open_default_font(float pt_size);

    TTF_TextEngine *text_engine_  = nullptr;
    TTF_Font       *default_font_ = nullptr;
};

} // namespace renderer
