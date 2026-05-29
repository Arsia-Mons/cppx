#pragma once

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

#include "../ui/runtime/draw_list.h"
#include "font_registry.h"

namespace renderer {

class SdlRetainedRenderer {
public:
  bool initialize(SDL_Renderer *renderer, FontRegistry &fonts);

  void clear(::ui::Color background);
  void render(const ::ui::legacy::DrawList &draw_list);
  void present();

  SDL_Renderer *sdl_renderer() const { return renderer_; }
  // The FontRegistry handed to initialize(); the new draw executor needs it to
  // rasterize text. May be null before initialize().
  FontRegistry *fonts() const { return fonts_; }

private:
  void render_rect(const ::ui::legacy::DrawCommand &command);
  void render_text(const ::ui::legacy::DrawCommand &command);

  SDL_Renderer *renderer_ = nullptr;
  FontRegistry *fonts_ = nullptr;
};

} // namespace renderer
