#pragma once

#include <SDL3/SDL.h>

#include <clay.h>
#include <clay_renderer_SDL3.h>

#include "font_registry.h"

namespace renderer {

class SdlClayRenderer {
public:
    bool initialize(SDL_Renderer *renderer, FontRegistry &fonts);

    void clear(Clay_Color background);
    void render(Clay_RenderCommandArray &commands);
    void present();

    SDL_Renderer *sdl_renderer() const { return renderer_; }

private:
    SDL_Renderer            *renderer_ = nullptr;
    TTF_Font                *fonts_[1] = { nullptr };
    Clay_SDL3RendererData    data_     = {};
};

} // namespace renderer
