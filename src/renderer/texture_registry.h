#pragma once

// texture_registry.h — the renderer-side texture_id -> SDL_Texture* map.
//
// This is the ONLY place a `ui/` texture_id (an opaque uint32_t carried in the
// IR's ImageData arm) meets a real SDL handle (design §16: "texture_id is an
// opaque uint32_t the renderer maps to its SDL_Texture* registry — the only
// place an SDL handle meets a ui/ id, on the renderer side"). `ui/` never sees
// SDL_Texture; the executor asks this registry for one by id.
//
// Textures are stored premultiplied (design §9.2): upload converts to
// premultiplied so the executor can draw them under
// SDL_BLENDMODE_BLEND_PREMULTIPLIED alongside all other geometry. The registry
// owns the SDL_Texture lifetimes and destroys them at shutdown.

#include <SDL3/SDL.h>

#include <stdint.h>

namespace renderer {

class TextureRegistry {
public:
  TextureRegistry() = default;
  ~TextureRegistry();

  TextureRegistry(const TextureRegistry &) = delete;
  TextureRegistry &operator=(const TextureRegistry &) = delete;

  // Adopts ownership of `texture` and returns a nonzero id the IR can carry in
  // ImageData::texture_id. The texture is expected to already be premultiplied
  // (SDL_BLENDMODE_BLEND_PREMULTIPLIED set by the caller, or set here). Returns
  // 0 if `texture` is null or the registry is full.
  uint32_t adopt(SDL_Texture *texture);

  // Uploads tightly-packed RGBA8888 straight-alpha pixels as a premultiplied
  // texture and returns its id (0 on failure). `renderer` is the live renderer
  // the texture is created against. The pixels are premultiplied at upload so
  // the executor draws under SDL_BLENDMODE_BLEND_PREMULTIPLIED.
  uint32_t upload_rgba(SDL_Renderer *renderer, const uint8_t *rgba, int width,
                       int height);

  // Maps an id back to its SDL_Texture* (nullptr for id==0 or unknown).
  SDL_Texture *lookup(uint32_t id) const;

  void shutdown();

private:
  static constexpr int kMaxTextures = 64;
  SDL_Texture *textures_[kMaxTextures] = {};
  int count_ = 0;
};

} // namespace renderer
