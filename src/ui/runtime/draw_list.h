#pragma once

#include "tree.h"

#include <array>
#include <stdint.h>

namespace ui {

// Legacy retained draw-list IR. Lives in ui::legacy so it can coexist in the
// same translation unit as the new tagged-union IR (ui::DrawCommand /
// ui::DrawCommandList in draw_command.h) during the dual-path window. This
// whole namespace is deleted when the legacy render path is pruned (step 7b).
namespace legacy {

constexpr int UI_RETAINED_MAX_DRAW_COMMANDS = UI_RETAINED_MAX_NODES * 2;

enum class DrawCommandKind : uint8_t {
  Rect,
  Text,
};

struct DrawCommand {
  DrawCommandKind kind = DrawCommandKind::Rect;
  NodeId node_id = 0;
  Rect rect = {};
  Color fill = {};
  Color border = {};
  float border_width = 0.0f;
  char text[UI_RETAINED_VALUE_CAP] = {};
  uint16_t font_size = 0;
};

struct DrawList {
  std::array<DrawCommand, UI_RETAINED_MAX_DRAW_COMMANDS> commands = {};
  int count = 0;
  int error_count = 0;

  bool push(const DrawCommand &command);
};

bool build_draw_list(const UiTree &tree, DrawList *out, NodeId focused_id = 0);

} // namespace legacy

} // namespace ui
