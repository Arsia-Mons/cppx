#pragma once

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

#include <stddef.h>
#include <stdint.h>

namespace renderer {

class FontRegistry {
public:
    FontRegistry() = default;
    ~FontRegistry();

    FontRegistry(const FontRegistry &) = delete;
    FontRegistry &operator=(const FontRegistry &) = delete;

    bool initialize(SDL_Renderer *renderer, float default_size = 16.0f);
    void shutdown();

    TTF_Font       *default_font() const { return default_font_; }

    // Rasterize `text` (len bytes) at `pixel_size` in STRAIGHT-alpha `color`,
    // returning a REGISTRY-OWNED SDL_Texture (do NOT destroy) sized *out_w x
    // *out_h. The result is cached and keyed by (bytes, pixel_size, color), so a
    // string that repeats across frames (the common case) is rasterized and
    // uploaded ONCE instead of every frame. Returns nullptr for empty/over-long
    // (>= 64 bytes) text or on failure — the caller renders those uncached.
    SDL_Texture *cached_text_texture(SDL_Renderer *renderer, const char *text,
                                     size_t len, int pixel_size, SDL_Color color,
                                     int *out_w, int *out_h);

private:
    static TTF_Font *open_default_font(float pt_size);
    void clear_text_cache();

    // Fixed-capacity LRU cache of rasterized text textures. Sized for a busy
    // screen's distinct (string,size,color) tuples; over-long strings bypass it.
    static constexpr int kTextCacheCap = 128;
    static constexpr int kTextKeyBytes = 64; // includes room for short labels
    struct TextEntry {
        bool        used = false;
        uint64_t    hash = 0;        // fast reject before the full compare
        int         len = 0;
        int         pixel_size = 0;
        SDL_Color   color = {0, 0, 0, 0};
        char        bytes[kTextKeyBytes] = {};
        SDL_Texture *tex = nullptr;
        int         w = 0;
        int         h = 0;
        uint64_t    last_used = 0;   // LRU tick
    };

    TTF_Font       *default_font_ = nullptr;
    TextEntry       text_cache_[kTextCacheCap] = {};
    uint64_t        text_clock_ = 0;
};

} // namespace renderer
