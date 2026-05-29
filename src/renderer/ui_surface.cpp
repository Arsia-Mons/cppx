#include "ui_surface.h"

namespace renderer {

bool UiSurface::initialize(SDL_Renderer *renderer, FontRegistry &fonts) {
  if (!renderer || !fonts.default_font())
    return false;
  renderer_ = renderer;
  fonts_ = &fonts;
  return true;
}

void UiSurface::clear(::ui::Color background) {
  if (!renderer_)
    return;
  SDL_SetRenderDrawColor(renderer_, background.r, background.g, background.b,
                         background.a);
  SDL_RenderClear(renderer_);
}

void UiSurface::present() {
  if (renderer_)
    SDL_RenderPresent(renderer_);
}

} // namespace renderer
