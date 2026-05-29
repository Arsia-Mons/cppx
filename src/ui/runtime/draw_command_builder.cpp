#include "draw_command_builder.h"

#include <string.h>

namespace ui {

namespace {

// Legacy fallback constants (straight alpha), reproduced from draw_list.cpp.
// Used only for not-yet-migrated nodes (dual-path); migrated nodes carry
// node.visual. Premultiplication happens at emit (premul()), never here.
constexpr Color kTransparent = {0, 0, 0, 0};
constexpr Color kButtonFill = {24, 28, 36, 255};
constexpr Color kButtonDisabledFill = {30, 34, 42, 255};
constexpr Color kButtonBorder = {78, 88, 104, 255};
constexpr Color kButtonDisabledBorder = {62, 68, 78, 255};
constexpr Color kFocusBorder = {122, 176, 238, 255};
constexpr float kFocusBorderWidth = 2.0f;
constexpr float kFocusBorderOffset = 2.0f;
constexpr Color kCheckedFill = {44, 92, 128, 255};
constexpr Color kInputFill = {18, 22, 28, 255};
constexpr Color kSelectionFill = {72, 116, 164, 180};
constexpr Color kCaretFill = {232, 240, 248, 255};
constexpr Color kTextFill = {226, 234, 242, 255};
constexpr Color kTextDisabledFill = {126, 134, 148, 255};

bool has_color(Color color) { return color.a > 0; }

// Straight-alpha -> premultiplied (the IR's storage convention). The only
// transparent color is (0,0,0,0); a==0 always maps to fully transparent.
Color premul(Color c) {
  return {
      static_cast<uint8_t>(static_cast<int>(c.r) * c.a / 255),
      static_cast<uint8_t>(static_cast<int>(c.g) * c.a / 255),
      static_cast<uint8_t>(static_cast<int>(c.b) * c.a / 255),
      c.a,
  };
}

DrawRect to_draw_rect(const Rect &r) {
  return {r.x, r.y, r.width, r.height};
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

bool push_text_command(DrawCommandList &list, NodeId node_id, const Rect &rect,
                       const char *value, Color straight_color,
                       uint16_t font_size, TextAlign align) {
  const char *safe = value ? value : "";
  uint32_t len = static_cast<uint32_t>(strlen(safe));
  uint16_t bytes = len > 0xFFFFu ? static_cast<uint16_t>(0xFFFFu)
                                 : static_cast<uint16_t>(len);
  uint32_t off = 0;
  if (!list.push_text(safe, bytes, &off))
    return false;

  DrawCommand command = {};
  command.kind = DrawCommandKind::Text;
  command.node_id = node_id;
  command.rect = to_draw_rect(rect);
  command.payload.text = {
      .text_off = off,
      .text_len = bytes,
      .color = premul(straight_color),
      .font_id = 0,
      .font_size = font_size,
      .line_index = 0,
      .align = align,
  };
  return list.push(command);
}

bool push_rect_command(DrawCommandList &list, NodeId node_id, const Rect &rect,
                       Color straight_fill, float corner_radius) {
  DrawCommand command = {};
  command.kind = DrawCommandKind::Rect;
  command.node_id = node_id;
  command.rect = to_draw_rect(rect);
  command.payload.rect = {
      .fill = premul(straight_fill),
      .corner_radius = corner_radius,
  };
  return list.push(command);
}

// FILL: emit a Rect command carrying only the resolved fill (the legacy
// append_rect fused fill + border + focus into one command; the new IR splits
// the stroke into a separate Border command, emitted by append_frame).
bool append_rect(DrawCommandList &list, const NodeSnapshot &node,
                 bool focused) {
  const VisualStyle &v = node.visual;
  bool visual_box = has_color(v.background) ||
                    (v.border.color.top.a > 0 && v.border.width.top > 0.0f) ||
                    (v.outline.color.a > 0 && v.outline.width > 0.0f);
  bool styled_box =
      has_color(node.style.background) ||
      (has_color(node.style.border) && node.style.border_width > 0.0f);
  bool control_box = node.role == NodeRole::Button ||
                     node.role == NodeRole::Checkbox ||
                     node.role == NodeRole::Input;
  if (!visual_box && !styled_box && !control_box && !focused) {
    return true;
  }

  Color fill =
      has_color(v.background)
          ? v.background
          : (has_color(node.style.background)
                 ? node.style.background
                 : (control_box ? control_fill(node) : kTransparent));

  return push_rect_command(list, node.id, node.layout, fill,
                           v.corner_radius);
}

// FRAME: the fused per-side border + signed-offset outline (focus ring). Mirror
// append_rect's source-selection: component-resolved border preferred, else the
// legacy single-color style/control border mapped to all four sides; outline is
// the component-resolved focus ring, else the legacy focus injection.
bool append_frame(DrawCommandList &list, const NodeSnapshot &node,
                  bool focused) {
  const VisualStyle &v = node.visual;
  bool control_box = node.role == NodeRole::Button ||
                     node.role == NodeRole::Checkbox ||
                     node.role == NodeRole::Input;

  // Border (per-side in the new IR; legacy single-color maps to all 4 sides).
  Border border = {};
  bool has_border = false;
  if (v.border.color.top.a > 0 && v.border.width.top > 0.0f) {
    border = v.border;
    has_border = true;
  } else {
    Color border_color = has_color(node.style.border)
                             ? node.style.border
                             : (node.interaction.disabled
                                    ? kButtonDisabledBorder
                                    : kButtonBorder);
    float border_width = node.style.border_width > 0.0f
                             ? node.style.border_width
                             : (control_box ? 1.0f : 0.0f);
    if (border_width > 0.0f) {
      border.width = {border_width, border_width, border_width, border_width};
      border.color = {border_color, border_color, border_color, border_color};
      has_border = true;
    }
  }

  // Outline / focus ring: prefer the component-resolved outline, else the
  // legacy any-source injection (focused && !disabled).
  Outline outline = {};
  bool has_outline = false;
  if (v.outline.color.a > 0 && v.outline.width > 0.0f) {
    outline = v.outline;
    has_outline = true;
  } else if (focused && !node.interaction.disabled) {
    outline = {kFocusBorderWidth, kFocusBorder, kFocusBorderOffset};
    has_outline = true;
  }

  if (!has_border && !has_outline)
    return true;

  // Premultiply every color into the IR.
  if (has_border) {
    border.color.top = premul(border.color.top);
    border.color.right = premul(border.color.right);
    border.color.bottom = premul(border.color.bottom);
    border.color.left = premul(border.color.left);
  }
  if (has_outline)
    outline.color = premul(outline.color);

  DrawCommand command = {};
  command.kind = DrawCommandKind::Border;
  command.node_id = node.id;
  command.rect = to_draw_rect(node.layout);
  command.payload.border = {
      .border = has_border ? border : Border{},
      .outline = has_outline ? outline : Outline{},
      .fill = {},
      .corner_radius = v.corner_radius,
      .has_fill = false,
      .has_outline = has_outline,
  };
  return list.push(command);
}

// TEXT (role==Text): single-line for this step (P4 makes it multi-line).
bool append_text(DrawCommandList &list, const NodeSnapshot &node,
                 bool inherited_disabled) {
  if (node.role != NodeRole::Text)
    return true;

  const VisualStyle &v = node.visual;
  Color color = has_color(v.text.color)
                    ? v.text.color
                    : (has_color(node.style.text)
                           ? node.style.text
                           : ((node.interaction.disabled || inherited_disabled)
                                  ? kTextDisabledFill
                                  : kTextFill));
  uint16_t font_size =
      v.text.font_size > 0
          ? v.text.font_size
          : (node.style.font_size > 0 ? node.style.font_size
                                      : static_cast<uint16_t>(15));
  TextAlign align = v.text.align;
  return push_text_command(list, node.id, node.layout, node.value, color,
                           font_size, align);
}

// INPUT contents (role==Input): selection rect, value text, caret rect. Keeps
// the legacy kInsetX/kCharWidth/kTextHeight geometry (P4 replaces kCharWidth
// with real measurement).
bool append_input_contents(DrawCommandList &list, const NodeSnapshot &node,
                           bool focused, bool inherited_disabled) {
  if (node.role != NodeRole::Input)
    return true;

  constexpr float kInsetX = 8.0f;
  constexpr float kCharWidth = 8.0f;
  constexpr float kTextHeight = 16.0f;
  float text_y = node.layout.y + (node.layout.height - kTextHeight) * 0.5f;
  Rect text_rect = {};
  text_rect.x = node.layout.x + kInsetX;
  text_rect.y = text_y;
  text_rect.width = node.layout.width - kInsetX * 2.0f;
  text_rect.height = kTextHeight;

  int length = text_length(node.value);
  int selection_start = clamp_int(node.text_edit.selection_start, 0, length);
  int selection_end = clamp_int(node.text_edit.selection_end, 0, length);
  if (selection_end < selection_start) {
    int tmp = selection_start;
    selection_start = selection_end;
    selection_end = tmp;
  }

  if (focused && selection_end > selection_start) {
    Rect sel = {};
    sel.x = text_rect.x + static_cast<float>(selection_start) * kCharWidth;
    sel.y = text_rect.y;
    sel.width =
        static_cast<float>(selection_end - selection_start) * kCharWidth;
    sel.height = text_rect.height;
    if (!push_rect_command(list, node.id, sel, kSelectionFill, 0.0f))
      return false;
  }

  Color text_color = (node.interaction.disabled || inherited_disabled)
                         ? kTextDisabledFill
                         : kTextFill;
  if (has_color(node.style.text))
    text_color = node.style.text;
  uint16_t font_size = node.style.font_size > 0 ? node.style.font_size
                                                : static_cast<uint16_t>(15);
  if (!push_text_command(list, node.id, text_rect, node.value, text_color,
                         font_size, TextAlign::Left))
    return false;

  if (focused) {
    int caret = clamp_int(node.text_edit.caret, 0, length);
    Rect caret_rect = {};
    caret_rect.x = text_rect.x + static_cast<float>(caret) * kCharWidth;
    caret_rect.y = text_rect.y - 1.0f;
    caret_rect.width = 1.0f;
    caret_rect.height = text_rect.height + 2.0f;
    if (!push_rect_command(list, node.id, caret_rect, kCaretFill, 0.0f))
      return false;
  }
  return true;
}

bool append_node(const UiTree &tree, DrawCommandList &list, NodeId id,
                 bool inherited_disabled, NodeId focused_id) {
  NodeSnapshot node = {};
  if (!tree.snapshot(id, &node))
    return false;

  bool disabled = inherited_disabled || node.interaction.disabled;
  bool focused = focused_id != 0 && focused_id == node.id;
  if (!append_rect(list, node, focused) ||
      !append_frame(list, node, focused) ||
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

bool build_draw_command_list(const UiTree &tree, DrawCommandList *out,
                             NodeId focused_id) {
  if (!out || !tree.contains(tree.root_id()))
    return false;
  out->reset();
  return append_node(tree, *out, tree.root_id(), false, focused_id) &&
         out->error_count == 0;
}

} // namespace ui
