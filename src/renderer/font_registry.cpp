#include "font_registry.h"

namespace renderer {

FontRegistry::~FontRegistry() {
    shutdown();
}

bool FontRegistry::initialize(SDL_Renderer *renderer, float default_size) {
    if (!renderer) return false;
    text_engine_ = TTF_CreateRendererTextEngine(renderer);
    if (!text_engine_) {
        SDL_Log("TTF_CreateRendererTextEngine: %s", SDL_GetError());
        return false;
    }
    default_font_ = open_default_font(default_size);
    if (!default_font_) {
        SDL_Log("no font found; tried system defaults");
        return false;
    }
    return true;
}

void FontRegistry::shutdown() {
    if (default_font_) {
        TTF_CloseFont(default_font_);
        default_font_ = nullptr;
    }
    if (text_engine_) {
        TTF_DestroyRendererTextEngine(text_engine_);
        text_engine_ = nullptr;
    }
}

TTF_Font *FontRegistry::open_default_font(float pt_size) {
    const char *candidates[] = {
        "/System/Library/Fonts/Helvetica.ttc",
        "/System/Library/Fonts/Supplemental/Arial.ttf",
        "/Library/Fonts/Arial.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/TTF/DejaVuSans.ttf",
        "C:\\Windows\\Fonts\\arial.ttf",
    };
    for (const char *path : candidates) {
        TTF_Font *font = TTF_OpenFont(path, pt_size);
        if (font) {
            SDL_Log("loaded font: %s", path);
            return font;
        }
    }
    return nullptr;
}

} // namespace renderer
