#include "sdl_clay_renderer.h"

namespace renderer {

bool SdlClayRenderer::initialize(SDL_Renderer *renderer, FontRegistry &fonts) {
    if (!renderer || !fonts.text_engine() || !fonts.default_font()) return false;
    renderer_   = renderer;
    fonts_[0]   = fonts.default_font();
    data_.renderer   = renderer_;
    data_.textEngine = fonts.text_engine();
    data_.fonts      = fonts_;
    return true;
}

void SdlClayRenderer::clear(Clay_Color background) {
    SDL_SetRenderDrawColor(renderer_,
                           (Uint8)background.r,
                           (Uint8)background.g,
                           (Uint8)background.b,
                           (Uint8)background.a);
    SDL_RenderClear(renderer_);
}

void SdlClayRenderer::render(Clay_RenderCommandArray &commands) {
    SDL_Clay_RenderClayCommands(&data_, &commands);
}

void SdlClayRenderer::present() {
    SDL_RenderPresent(renderer_);
}

} // namespace renderer
