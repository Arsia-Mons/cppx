#include "yoga_flex_layout.h"

#include <yoga/Yoga.h>

namespace ui::retained {

namespace {

YGFlexDirection map_direction(FlexDirection direction) {
  switch (direction) {
  case FlexDirection::Row:
    return YGFlexDirectionRow;
  case FlexDirection::Column:
    return YGFlexDirectionColumn;
  }
  return YGFlexDirectionColumn;
}

YGAlign map_align(AlignItems align) {
  switch (align) {
  case AlignItems::Stretch:
    return YGAlignStretch;
  case AlignItems::Start:
    return YGAlignFlexStart;
  case AlignItems::Center:
    return YGAlignCenter;
  case AlignItems::End:
    return YGAlignFlexEnd;
  }
  return YGAlignStretch;
}

YGJustify map_justify(JustifyContent justify) {
  switch (justify) {
  case JustifyContent::Start:
    return YGJustifyFlexStart;
  case JustifyContent::Center:
    return YGJustifyCenter;
  case JustifyContent::End:
    return YGJustifyFlexEnd;
  case JustifyContent::SpaceBetween:
    return YGJustifySpaceBetween;
  }
  return YGJustifyFlexStart;
}

void set_width(YGNodeRef node, Length length) {
  switch (length.kind) {
  case LengthKind::Auto:
    YGNodeStyleSetWidthAuto(node);
    break;
  case LengthKind::Points:
    YGNodeStyleSetWidth(node, length.value);
    break;
  case LengthKind::Percent:
    YGNodeStyleSetWidthPercent(node, length.value);
    break;
  case LengthKind::Grow:
    YGNodeStyleSetWidthAuto(node);
    YGNodeStyleSetFlexGrow(node, length.value);
    break;
  }
}

void set_height(YGNodeRef node, Length length) {
  switch (length.kind) {
  case LengthKind::Auto:
    YGNodeStyleSetHeightAuto(node);
    break;
  case LengthKind::Points:
    YGNodeStyleSetHeight(node, length.value);
    break;
  case LengthKind::Percent:
    YGNodeStyleSetHeightPercent(node, length.value);
    break;
  case LengthKind::Grow:
    YGNodeStyleSetHeightAuto(node);
    YGNodeStyleSetFlexGrow(node, length.value);
    break;
  }
}

void apply_style(YGNodeRef yoga_node, const NodeSnapshot &snapshot,
                 LayoutViewport viewport) {
  const Style &style = snapshot.style;
  YGNodeStyleSetBoxSizing(yoga_node, YGBoxSizingBorderBox);
  YGNodeStyleSetFlexDirection(yoga_node, map_direction(style.direction));
  YGNodeStyleSetAlignItems(yoga_node, map_align(style.align_items));
  YGNodeStyleSetJustifyContent(yoga_node, map_justify(style.justify_content));
  YGNodeStyleSetPadding(yoga_node, YGEdgeLeft, style.padding.left);
  YGNodeStyleSetPadding(yoga_node, YGEdgeRight, style.padding.right);
  YGNodeStyleSetPadding(yoga_node, YGEdgeTop, style.padding.top);
  YGNodeStyleSetPadding(yoga_node, YGEdgeBottom, style.padding.bottom);
  YGNodeStyleSetGap(yoga_node, YGGutterAll, style.gap);

  if (snapshot.id == UI_RETAINED_ROOT_ID) {
    YGNodeStyleSetWidth(yoga_node, viewport.width);
    YGNodeStyleSetHeight(yoga_node, viewport.height);
    return;
  }

  set_width(yoga_node, style.width);
  set_height(yoga_node, style.height);
  if (style.flex_grow > 0.0f)
    YGNodeStyleSetFlexGrow(yoga_node, style.flex_grow);
}

YGNodeRef build_yoga_tree(UiTree &tree, NodeId id, LayoutViewport viewport) {
  NodeSnapshot snapshot = {};
  if (!tree.snapshot(id, &snapshot))
    return nullptr;

  YGNodeRef yoga_node = YGNodeNew();
  if (!yoga_node)
    return nullptr;

  apply_style(yoga_node, snapshot, viewport);

  for (int i = 0; i < tree.child_count(id); ++i) {
    YGNodeRef child = build_yoga_tree(tree, tree.child_at(id, i), viewport);
    if (!child) {
      YGNodeFreeRecursive(yoga_node);
      return nullptr;
    }
    YGNodeInsertChild(yoga_node, child, static_cast<uint32_t>(i));
  }

  return yoga_node;
}

bool write_layout(UiTree &tree, NodeId id, YGNodeRef yoga_node, float parent_x,
                  float parent_y) {
  if (!yoga_node)
    return false;

  float x = parent_x + YGNodeLayoutGetLeft(yoga_node);
  float y = parent_y + YGNodeLayoutGetTop(yoga_node);
  if (!tree.set_layout(id, {x, y, YGNodeLayoutGetWidth(yoga_node),
                            YGNodeLayoutGetHeight(yoga_node)})) {
    return false;
  }

  int children = tree.child_count(id);
  for (int i = 0; i < children; ++i) {
    YGNodeRef yoga_child = YGNodeGetChild(yoga_node, static_cast<uint32_t>(i));
    if (!write_layout(tree, tree.child_at(id, i), yoga_child, x, y))
      return false;
  }
  return true;
}

bool compute_yoga_layout(UiTree &tree, NodeId root_id, LayoutViewport viewport,
                         void *) {
  YGNodeRef root = build_yoga_tree(tree, root_id, viewport);
  if (!root)
    return false;

  YGNodeCalculateLayout(root, viewport.width, viewport.height, YGDirectionLTR);
  bool ok = write_layout(tree, root_id, root, 0.0f, 0.0f);
  YGNodeFreeRecursive(root);
  return ok;
}

} // namespace

FlexLayoutAdapter make_yoga_flex_layout_adapter() {
  return {
      .name = "Yoga",
      .compute = compute_yoga_layout,
      .user = nullptr,
  };
}

} // namespace ui::retained
