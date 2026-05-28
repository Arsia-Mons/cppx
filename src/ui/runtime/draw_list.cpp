#include "draw_list.h"

#include <string.h>

namespace ui {

namespace {

constexpr Color kTransparent = {0, 0, 0, 0};
constexpr Color kButtonFill = {24, 28, 36, 255};
constexpr Color kButtonDisabledFill = {30, 34, 42, 255};
constexpr Color kButtonBorder = {78, 88, 104, 255};
constexpr Color kButtonDisabledBorder = {62, 68, 78, 255};
constexpr Color kCheckedFill = {44, 92, 128, 255};
constexpr Color kInputFill = {18, 22, 28, 255};
constexpr Color kSelectionFill = {72, 116, 164, 180};
constexpr Color kCaretFill = {232, 240, 248, 255};
constexpr Color kTextFill = {226, 234, 242, 255};
constexpr Color kTextDisabledFill = {126, 134, 148, 255};

bool has_color(Color color) { return color.a > 0; }

void copy_text(char (&dest)[UI_RETAINED_VALUE_CAP], const char *source) {
  const char *safe_source = source ? source : "";
  strncpy(dest, safe_source, UI_RETAINED_VALUE_CAP - 1);
  dest[UI_RETAINED_VALUE_CAP - 1] = '\0';
}

Color control_fill(const NodeSnapshot &node) {
  if (node.interaction.disabled)
    return kButtonDisabledFill;
  if (node.role == NodeRole::Checkbox && node.interaction.checked)
    return kCheckedFill;
  if (node.role == NodeRole::Input)
    return kInputFill;
  return kButtonFill;
}

bool append_rect(DrawList &list, const NodeSnapshot &node) {
  bool styled_box =
      has_color(node.style.background) ||
      (has_color(node.style.border) && node.style.border_width > 0.0f);
  bool control_box = node.role == NodeRole::Button ||
                     node.role == NodeRole::Checkbox ||
                     node.role == NodeRole::Input;
  if (!styled_box && !control_box) {
    return true;
  }

  return list.push({
      .kind = DrawCommandKind::Rect,
      .node_id = node.id,
      .rect = node.layout,
      .fill = has_color(node.style.background)
                  ? node.style.background
                  : (control_box ? control_fill(node) : kTransparent),
      .border = has_color(node.style.border)
                    ? node.style.border
                    : (node.interaction.disabled ? kButtonDisabledBorder
                                                 : kButtonBorder),
      .border_width = node.style.border_width > 0.0f
                          ? node.style.border_width
                          : (control_box ? 1.0f : 0.0f),
  });
}

bool append_text(DrawList &list, const NodeSnapshot &node,
                 bool inherited_disabled) {
  if (node.role != NodeRole::Text)
    return true;

  DrawCommand command = {
      .kind = DrawCommandKind::Text,
      .node_id = node.id,
      .rect = node.layout,
      .fill = has_color(node.style.text)
                  ? node.style.text
                  : ((node.interaction.disabled || inherited_disabled)
                         ? kTextDisabledFill
                         : kTextFill),
      .border = kTransparent,
      .font_size = node.style.font_size > 0 ? node.style.font_size
                                            : static_cast<uint16_t>(15),
  };
  copy_text(command.text, node.value);
  return list.push(command);
}

int text_length(const char *value) {
  return static_cast<int>(strlen(value ? value : ""));
}

int clamp_int(int value, int low, int high) {
  if (value < low)
    return low;
  if (value > high)
    return high;
  return value;
}

bool append_input_contents(DrawList &list, const NodeSnapshot &node,
                           bool focused, bool inherited_disabled) {
  if (node.role != NodeRole::Input)
    return true;

  constexpr float kInsetX = 8.0f;
  constexpr float kCharWidth = 8.0f;
  constexpr float kTextHeight = 16.0f;
  float text_y = node.layout.y + (node.layout.height - kTextHeight) * 0.5f;
  Rect text_rect = {
      .x = node.layout.x + kInsetX,
      .y = text_y,
      .width = node.layout.width - kInsetX * 2.0f,
      .height = kTextHeight,
  };

  int length = text_length(node.value);
  int selection_start = clamp_int(node.text_edit.selection_start, 0, length);
  int selection_end = clamp_int(node.text_edit.selection_end, 0, length);
  if (selection_end < selection_start) {
    int tmp = selection_start;
    selection_start = selection_end;
    selection_end = tmp;
  }

  if (focused && selection_end > selection_start) {
    if (!list.push({
            .kind = DrawCommandKind::Rect,
            .node_id = node.id,
            .rect =
                {
                    .x = text_rect.x +
                         static_cast<float>(selection_start) * kCharWidth,
                    .y = text_rect.y,
                    .width =
                        static_cast<float>(selection_end - selection_start) *
                        kCharWidth,
                    .height = text_rect.height,
                },
            .fill = kSelectionFill,
        })) {
      return false;
    }
  }

  DrawCommand text = {
      .kind = DrawCommandKind::Text,
      .node_id = node.id,
      .rect = text_rect,
      .fill = (node.interaction.disabled || inherited_disabled)
                  ? kTextDisabledFill
                  : kTextFill,
      .font_size = node.style.font_size > 0 ? node.style.font_size
                                            : static_cast<uint16_t>(15),
  };
  if (has_color(node.style.text))
    text.fill = node.style.text;
  copy_text(text.text, node.value);
  if (!list.push(text))
    return false;

  if (focused) {
    int caret = clamp_int(node.text_edit.caret, 0, length);
    if (!list.push({
            .kind = DrawCommandKind::Rect,
            .node_id = node.id,
            .rect =
                {
                    .x = text_rect.x + static_cast<float>(caret) * kCharWidth,
                    .y = text_rect.y - 1.0f,
                    .width = 1.0f,
                    .height = text_rect.height + 2.0f,
                },
            .fill = kCaretFill,
        })) {
      return false;
    }
  }
  return true;
}

bool append_node(const UiTree &tree, DrawList &list, NodeId id,
                 bool inherited_disabled, NodeId focused_id) {
  NodeSnapshot node = {};
  if (!tree.snapshot(id, &node))
    return false;

  bool disabled = inherited_disabled || node.interaction.disabled;
  bool focused = focused_id != 0 && focused_id == node.id;
  if (!append_rect(list, node) ||
      !append_text(list, node, inherited_disabled) ||
      !append_input_contents(list, node, focused, inherited_disabled))
    return false;

  for (int i = 0; i < tree.child_count(id); ++i) {
    if (!append_node(tree, list, tree.child_at(id, i), disabled, focused_id))
      return false;
  }
  return true;
}

} // namespace

bool DrawList::push(const DrawCommand &command) {
  if (count >= UI_RETAINED_MAX_DRAW_COMMANDS) {
    ++error_count;
    return false;
  }
  commands[count++] = command;
  return true;
}

bool build_draw_list(const UiTree &tree, DrawList *out, NodeId focused_id) {
  if (!out || !tree.contains(tree.root_id()))
    return false;
  *out = {};
  return append_node(tree, *out, tree.root_id(), false, focused_id) &&
         out->error_count == 0;
}

} // namespace ui
