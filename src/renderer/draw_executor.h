#pragma once

// Linear executor over the new tagged-union DrawCommand IR (draw_command.h).
// Consumes a DrawCommandList and emits SDL draw calls — the P3 renderer.
// Colors in the IR are premultiplied (design §8.4), so geometry is drawn under
// SDL_BLENDMODE_BLEND_PREMULTIPLIED. `fonts` may be null (text is then skipped),
// which keeps geometry goldens font-independent.

#include "ui/runtime/draw_command.h"

#include <SDL3/SDL.h>

namespace renderer {

class FontRegistry;

void execute_draw_commands(SDL_Renderer *renderer,
                           const ::ui::DrawCommandList &list,
                           FontRegistry *fonts);

} // namespace renderer
