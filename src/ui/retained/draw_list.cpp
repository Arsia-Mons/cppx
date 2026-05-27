#include "draw_list.h"

#include <string.h>

namespace ui::retained {

namespace {

constexpr Color kTransparent = {0, 0, 0, 0};
constexpr Color kButtonFill = {24, 28, 36, 255};
constexpr Color kButtonDisabledFill = {30, 34, 42, 255};
constexpr Color kButtonBorder = {78, 88, 104, 255};
constexpr Color kButtonDisabledBorder = {62, 68, 78, 255};
constexpr Color kToggleCheckedFill = {44, 92, 128, 255};
constexpr Color kSelectableSelectedFill = {42, 80, 60, 255};
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
  if (node.role == NodeRole::Toggle && node.interaction.checked)
    return kToggleCheckedFill;
  if (node.role == NodeRole::Selectable && node.interaction.selected)
    return kSelectableSelectedFill;
  return kButtonFill;
}

bool append_rect(DrawList &list, const NodeSnapshot &node) {
  bool styled_box = has_color(node.visual.background) ||
                    (has_color(node.visual.border) &&
                     node.visual.border_width > 0.0f);
  bool control_box = node.role == NodeRole::Button ||
                     node.role == NodeRole::Toggle ||
                     node.role == NodeRole::Selectable;
  if (!styled_box && !control_box) {
    return true;
  }

  return list.push({
      .kind = DrawCommandKind::Rect,
      .node_id = node.id,
      .rect = node.layout,
      .fill = has_color(node.visual.background)
                  ? node.visual.background
                  : (control_box ? control_fill(node) : kTransparent),
      .border = has_color(node.visual.border)
                    ? node.visual.border
                    : (node.interaction.disabled ? kButtonDisabledBorder
                                                 : kButtonBorder),
      .border_width = node.visual.border_width > 0.0f
                          ? node.visual.border_width
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
      .fill = has_color(node.visual.text)
                  ? node.visual.text
                  : ((node.interaction.disabled || inherited_disabled)
                         ? kTextDisabledFill
                         : kTextFill),
      .border = kTransparent,
      .font_size = node.visual.font_size > 0
                       ? node.visual.font_size
                       : static_cast<uint16_t>(15),
  };
  copy_text(command.text, node.value);
  return list.push(command);
}

bool append_node(const UiTree &tree, DrawList &list, NodeId id,
                 bool inherited_disabled) {
  NodeSnapshot node = {};
  if (!tree.snapshot(id, &node))
    return false;

  bool disabled = inherited_disabled || node.interaction.disabled;
  if (!append_rect(list, node) || !append_text(list, node, inherited_disabled))
    return false;

  for (int i = 0; i < tree.child_count(id); ++i) {
    if (!append_node(tree, list, tree.child_at(id, i), disabled))
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

bool build_draw_list(const UiTree &tree, DrawList *out) {
  if (!out || !tree.contains(tree.root_id()))
    return false;
  *out = {};
  return append_node(tree, *out, tree.root_id(), false) &&
         out->error_count == 0;
}

} // namespace ui::retained
