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

#include "ui/runtime/draw_command.h"

#include <SDL3/SDL.h>

namespace renderer {

class FontRegistry;
class TextureRegistry;

void execute_draw_commands(SDL_Renderer *renderer,
                           const ::ui::DrawCommandList &list,
                           FontRegistry *fonts,
                           TextureRegistry *textures = nullptr,
                           float scale = 1.0f);

} // namespace renderer
