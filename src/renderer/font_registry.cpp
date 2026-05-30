#include "font_registry.h"

#include <string.h>

namespace renderer {

namespace {

// FNV-1a over the cache key (bytes + size + color) — a fast reject hash; the
// lookup still confirms with a full field compare so collisions never matter.
uint64_t key_hash(const char *text, size_t len, int pixel_size, SDL_Color c) {
    uint64_t h = 1469598103934665603ull;
    auto mix = [&](uint8_t b) { h ^= b; h *= 1099511628211ull; };
    for (size_t i = 0; i < len; ++i) mix(static_cast<uint8_t>(text[i]));
    mix(static_cast<uint8_t>(pixel_size));
    mix(static_cast<uint8_t>(pixel_size >> 8));
    mix(c.r); mix(c.g); mix(c.b); mix(c.a);
    return h;
}

} // namespace

FontRegistry::~FontRegistry() {
    shutdown();
}

bool FontRegistry::initialize(SDL_Renderer *renderer, float default_size) {
    if (!renderer) return false;
    default_font_ = open_default_font(default_size);
    if (!default_font_) {
        SDL_Log("no font found; tried system defaults");
        return false;
    }
    return true;
}

void FontRegistry::clear_text_cache() {
    for (TextEntry &e : text_cache_) {
        if (e.tex) {
            SDL_DestroyTexture(e.tex);
            e.tex = nullptr;
        }
        e.used = false;
    }
}

void FontRegistry::shutdown() {
    clear_text_cache(); // free cached textures before the renderer is destroyed
    if (default_font_) {
        TTF_CloseFont(default_font_);
        default_font_ = nullptr;
    }
}

SDL_Texture *FontRegistry::cached_text_texture(SDL_Renderer *renderer,
                                               const char *text, size_t len,
                                               int pixel_size, SDL_Color color,
                                               int *out_w, int *out_h) {
    if (!renderer || !default_font_ || !text || len == 0 ||
        len >= static_cast<size_t>(kTextKeyBytes) || pixel_size <= 0)
        return nullptr;

    ++text_clock_;
    const uint64_t h = key_hash(text, len, pixel_size, color);

    // Lookup: fast hash reject, then a full field compare (no collision risk).
    int lru = 0;
    uint64_t lru_tick = UINT64_MAX;
    for (int i = 0; i < kTextCacheCap; ++i) {
        TextEntry &e = text_cache_[i];
        if (!e.used) {
            if (lru_tick != 0) { lru = i; lru_tick = 0; } // prefer a free slot
            continue;
        }
        if (e.hash == h && e.len == static_cast<int>(len) &&
            e.pixel_size == pixel_size && e.color.r == color.r &&
            e.color.g == color.g && e.color.b == color.b &&
            e.color.a == color.a && memcmp(e.bytes, text, len) == 0) {
            e.last_used = text_clock_;
            if (out_w) *out_w = e.w;
            if (out_h) *out_h = e.h;
            return e.tex;
        }
        if (e.last_used < lru_tick) { lru_tick = e.last_used; lru = i; }
    }

    // Miss: rasterize once at the requested device pixel size + straight color.
    TTF_SetFontSize(default_font_, static_cast<float>(pixel_size));
    SDL_Surface *surface = TTF_RenderText_Blended(default_font_, text, len, color);
    if (!surface) return nullptr;
    SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surface);
    const int tw = surface->w, th = surface->h;
    SDL_DestroySurface(surface);
    if (!tex) return nullptr;

    // Install into the chosen slot (free or LRU-evicted).
    TextEntry &e = text_cache_[lru];
    if (e.tex) SDL_DestroyTexture(e.tex);
    e.used = true;
    e.hash = h;
    e.len = static_cast<int>(len);
    e.pixel_size = pixel_size;
    e.color = color;
    memcpy(e.bytes, text, len);
    e.tex = tex;
    e.w = tw;
    e.h = th;
    e.last_used = text_clock_;
    if (out_w) *out_w = tw;
    if (out_h) *out_h = th;
    return tex;
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
