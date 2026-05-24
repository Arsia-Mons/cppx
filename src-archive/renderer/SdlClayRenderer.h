#pragma once

#include "renderer/FontRegistry.h"

#include <SDL3/SDL.h>
#include <clay.h>
#include <clay_renderer_SDL3.h>

namespace renderer {

class SdlClayRenderer {
public:
    SdlClayRenderer(SDL_Renderer *renderer, FontRegistry &fonts);

    void render(Clay_RenderCommandArray &commands);

private:
    Clay_SDL3RendererData rendererData_{};
};

}
