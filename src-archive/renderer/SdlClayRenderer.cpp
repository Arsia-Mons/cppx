#include "renderer/SdlClayRenderer.h"

namespace renderer {

SdlClayRenderer::SdlClayRenderer(SDL_Renderer *renderer, FontRegistry &fonts)
    : rendererData_{.renderer = renderer, .textEngine = fonts.textEngine(), .fonts = fonts.fonts()} {}

void SdlClayRenderer::render(Clay_RenderCommandArray &commands) {
    SDL_Clay_RenderClayCommands(&rendererData_, &commands);
}

}
