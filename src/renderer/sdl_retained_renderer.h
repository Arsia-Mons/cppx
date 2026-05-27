#pragma once

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

#include "../ui/retained/draw_list.h"
#include "font_registry.h"

namespace renderer {

class SdlRetainedRenderer {
public:
    bool initialize(SDL_Renderer *renderer, FontRegistry &fonts);

    void render(const ::ui::retained::DrawList &draw_list);

private:
    void render_rect(const ::ui::retained::DrawCommand &command);
    void render_text(const ::ui::retained::DrawCommand &command);

    SDL_Renderer *renderer_ = nullptr;
    FontRegistry *fonts_ = nullptr;
};

} // namespace renderer
