#pragma once

// Linear executor over the new tagged-union DrawCommand IR (draw_command.h).
// Consumes a DrawCommandList and emits SDL draw calls — the P3 renderer.
// Colors in the IR are premultiplied (design §8.4), so geometry is drawn under
// SDL_BLENDMODE_BLEND_PREMULTIPLIED. `fonts` may be null (text is then skipped)
// and `textures` may be null (Image commands are then skipped), which keeps
// geometry goldens font- and texture-independent.
//
// `scale` is device pixels per UI point (the window's pixel density). The IR is
// authored in points; the executor multiplies every emitted coordinate by
// `scale` and renders glyphs / feathers at the scaled resolution, so the UI
// fills the native-resolution backbuffer crisply (HiDPI). scale==1 reproduces
// the legacy point==pixel path exactly (headless goldens are unaffected).

#include "render_mode.h"
#include "ui/runtime/draw_command.h"

#include <SDL3/SDL.h>

namespace renderer {

class FontRegistry;
class TextureRegistry;
class SdfMaskCache;

// `mode` selects how rounded vector primitives (fills, borders, gradients) are
// rasterized (see render_mode.h). The default — FringeAa at scale 1 — is exactly
// the legacy per-primitive feather path, so existing call sites and goldens are
// unchanged. SSAA mode emits hard-edged geometry (its AA comes from a
// full-scene supersample done by UiSurface, NOT here). SDF mode routes rounded
// shapes through `sdf_cache` (a null cache still works — masks are then
// generated transiently per call). Shadows, images, text, clips and layers are
// mode-independent and shared across all three.
void execute_draw_commands(SDL_Renderer *renderer,
                           const ::ui::DrawCommandList &list,
                           FontRegistry *fonts,
                           TextureRegistry *textures = nullptr,
                           float scale = 1.0f,
                           RenderMode mode = RenderMode::FringeAa,
                           SdfMaskCache *sdf_cache = nullptr);

} // namespace renderer
