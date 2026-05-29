#pragma once

// build_draw_command_list: the tree -> new tagged-union DrawCommandList IR
// transcriber (styling/render design §8). Walks the retained UiTree depth-first
// in the same order as the legacy build_draw_list (draw_list.cpp) and emits the
// new IR (draw_command.h). Semantically equivalent to the legacy builder, but
// targets the new IR: colors are PREMULTIPLIED at emit, variable data goes to
// the list's arenas, and box paint is split into a Rect (fill) command plus a
// fused Border command (border + focus outline).
//
// This lands ADDITIVELY alongside build_draw_list (dual-path). The live render
// path keeps calling build_draw_list until the executor is flipped.

#include "draw_command.h"
#include "tree.h"

namespace ui {

bool build_draw_command_list(const UiTree &tree, DrawCommandList *out,
                             NodeId focused_id = 0);

} // namespace ui
