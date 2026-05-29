#pragma once

#include <SDL3/SDL.h>

#include "../ui/style/visual_style.h" // ::ui::Color
#include "font_registry.h"

namespace renderer {

// Minimal SDL surface the per-frame loop draws into: clear -> execute the
// DrawCommand IR via execute_draw_commands -> present. Owns no draw IR itself
// (the new tagged-union IR lives in client::ui); it only exposes the SDL_Renderer
// and the FontRegistry the executor needs. Replaces the deleted
// SdlRetainedRenderer (legacy DrawList path) — renderer/ stays free of
// client/game deps.
class UiSurface {
public:
  bool initialize(SDL_Renderer *renderer, FontRegistry &fonts);

  void clear(::ui::Color background);
  void present();

  SDL_Renderer *sdl_renderer() const { return renderer_; }
  // The FontRegistry handed to initialize(); the draw executor needs it to
  // rasterize text. May be null before initialize().
  FontRegistry *fonts() const { return fonts_; }

private:
  SDL_Renderer *renderer_ = nullptr;
  FontRegistry *fonts_ = nullptr;
};

} // namespace renderer
