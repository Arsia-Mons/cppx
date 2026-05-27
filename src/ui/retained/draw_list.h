#pragma once

#include "ui_tree.h"

#include <array>
#include <stdint.h>

namespace ui::retained {

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

bool build_draw_list(const UiTree &tree, DrawList *out);

} // namespace ui::retained
