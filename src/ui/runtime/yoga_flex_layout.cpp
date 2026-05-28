#include "yoga_flex_layout.h"

#include <yoga/Yoga.h>

namespace ui::retained {

namespace {

struct MeasureContext {
  UiTree *tree = nullptr;
  NodeId id = 0;
};

struct BuildContext {
  std::array<MeasureContext, UI_RETAINED_MAX_NODES> measure_contexts = {};
  int measure_context_count = 0;
};

MeasureMode map_measure_mode(YGMeasureMode mode) {
  switch (mode) {
  case YGMeasureModeUndefined:
    return MeasureMode::Undefined;
  case YGMeasureModeExactly:
    return MeasureMode::Exactly;
  case YGMeasureModeAtMost:
    return MeasureMode::AtMost;
  }
  return MeasureMode::Undefined;
}

YGSize measure_yoga_node(YGNodeConstRef node, float width,
                         YGMeasureMode width_mode, float height,
                         YGMeasureMode height_mode) {
  const MeasureContext *context =
      static_cast<const MeasureContext *>(YGNodeGetContext(node));
  if (!context || !context->tree)
    return {0.0f, 0.0f};

  Size measured = {};
  if (!context->tree->measure(context->id,
                              {
                                  .width = width,
                                  .height = height,
                                  .width_mode = map_measure_mode(width_mode),
                                  .height_mode = map_measure_mode(height_mode),
                              },
                              &measured)) {
    return {0.0f, 0.0f};
  }
  return {measured.width, measured.height};
}

YGFlexDirection map_direction(FlexDirection direction) {
  switch (direction) {
  case FlexDirection::Row:
    return YGFlexDirectionRow;
  case FlexDirection::RowReverse:
    return YGFlexDirectionRowReverse;
  case FlexDirection::Column:
    return YGFlexDirectionColumn;
  case FlexDirection::ColumnReverse:
    return YGFlexDirectionColumnReverse;
  }
  return YGFlexDirectionColumn;
}

YGAlign map_align(AlignItems align) {
  switch (align) {
  case AlignItems::Auto:
    return YGAlignAuto;
  case AlignItems::Stretch:
    return YGAlignStretch;
  case AlignItems::Start:
    return YGAlignFlexStart;
  case AlignItems::Center:
    return YGAlignCenter;
  case AlignItems::End:
    return YGAlignFlexEnd;
  case AlignItems::Baseline:
    return YGAlignBaseline;
  case AlignItems::SpaceBetween:
    return YGAlignSpaceBetween;
  case AlignItems::SpaceAround:
    return YGAlignSpaceAround;
  case AlignItems::SpaceEvenly:
    return YGAlignSpaceEvenly;
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
  case JustifyContent::SpaceAround:
    return YGJustifySpaceAround;
  case JustifyContent::SpaceEvenly:
    return YGJustifySpaceEvenly;
  }
  return YGJustifyFlexStart;
}

YGWrap map_wrap(FlexWrap wrap) {
  switch (wrap) {
  case FlexWrap::NoWrap:
    return YGWrapNoWrap;
  case FlexWrap::Wrap:
    return YGWrapWrap;
  case FlexWrap::WrapReverse:
    return YGWrapWrapReverse;
  }
  return YGWrapNoWrap;
}

YGPositionType map_position(PositionType position) {
  switch (position) {
  case PositionType::Static:
    return YGPositionTypeStatic;
  case PositionType::Relative:
    return YGPositionTypeRelative;
  case PositionType::Absolute:
    return YGPositionTypeAbsolute;
  }
  return YGPositionTypeRelative;
}

YGOverflow map_overflow(Overflow overflow) {
  switch (overflow) {
  case Overflow::Visible:
    return YGOverflowVisible;
  case Overflow::Hidden:
    return YGOverflowHidden;
  case Overflow::Scroll:
    return YGOverflowScroll;
  }
  return YGOverflowVisible;
}

YGDisplay map_display(Display display) {
  switch (display) {
  case Display::Flex:
    return YGDisplayFlex;
  case Display::None:
    return YGDisplayNone;
  case Display::Contents:
    return YGDisplayContents;
  }
  return YGDisplayFlex;
}

void set_dimension(YGNodeRef node, Length length,
                   void (*set_points)(YGNodeRef, float),
                   void (*set_percent)(YGNodeRef, float),
                   void (*set_auto)(YGNodeRef)) {
  switch (length.kind) {
  case LengthKind::Auto:
    set_auto(node);
    break;
  case LengthKind::Points:
    set_points(node, length.value);
    break;
  case LengthKind::Percent:
    set_percent(node, length.value);
    break;
  case LengthKind::Grow:
    set_auto(node);
    YGNodeStyleSetFlexGrow(node, length.value);
    break;
  }
}

void set_optional_length(YGNodeRef node, Length length,
                         void (*set_points)(YGNodeRef, float),
                         void (*set_percent)(YGNodeRef, float)) {
  switch (length.kind) {
  case LengthKind::Auto:
    break;
  case LengthKind::Points:
    set_points(node, length.value);
    break;
  case LengthKind::Percent:
    set_percent(node, length.value);
    break;
  case LengthKind::Grow:
    YGNodeStyleSetFlexGrow(node, length.value);
    break;
  }
}

void set_flex_basis(YGNodeRef node, Length length) {
  switch (length.kind) {
  case LengthKind::Auto:
    YGNodeStyleSetFlexBasisAuto(node);
    break;
  case LengthKind::Points:
    YGNodeStyleSetFlexBasis(node, length.value);
    break;
  case LengthKind::Percent:
    YGNodeStyleSetFlexBasisPercent(node, length.value);
    break;
  case LengthKind::Grow:
    YGNodeStyleSetFlexBasisAuto(node);
    YGNodeStyleSetFlexGrow(node, length.value);
    break;
  }
}

void set_edges(YGNodeRef node, const EdgeSizes &edges,
               void (*set_edge)(YGNodeRef, YGEdge, float)) {
  set_edge(node, YGEdgeLeft, edges.left);
  set_edge(node, YGEdgeRight, edges.right);
  set_edge(node, YGEdgeTop, edges.top);
  set_edge(node, YGEdgeBottom, edges.bottom);
}

void apply_style(YGNodeRef yoga_node, const NodeSnapshot &snapshot,
                 LayoutViewport viewport) {
  const Style &style = snapshot.style;
  YGNodeStyleSetBoxSizing(yoga_node, YGBoxSizingBorderBox);
  YGNodeStyleSetDisplay(yoga_node, map_display(style.display));
  YGNodeStyleSetPositionType(yoga_node, map_position(style.position));
  YGNodeStyleSetOverflow(yoga_node, map_overflow(style.overflow));
  YGNodeStyleSetFlexDirection(yoga_node, map_direction(style.direction));
  YGNodeStyleSetFlexWrap(yoga_node, map_wrap(style.wrap));
  YGNodeStyleSetAlignItems(yoga_node, map_align(style.align_items));
  YGNodeStyleSetAlignContent(yoga_node, map_align(style.align_content));
  YGNodeStyleSetAlignSelf(yoga_node, map_align(style.align_self));
  YGNodeStyleSetJustifyContent(yoga_node, map_justify(style.justify_content));
  set_edges(yoga_node, style.margin, YGNodeStyleSetMargin);
  set_edges(yoga_node, style.padding, YGNodeStyleSetPadding);
  set_edges(yoga_node, style.position_inset, YGNodeStyleSetPosition);
  YGNodeStyleSetBorder(yoga_node, YGEdgeAll, style.border_width);
  YGNodeStyleSetGap(yoga_node, YGGutterAll, style.gap);
  if (style.row_gap > 0.0f)
    YGNodeStyleSetGap(yoga_node, YGGutterRow, style.row_gap);
  if (style.column_gap > 0.0f)
    YGNodeStyleSetGap(yoga_node, YGGutterColumn, style.column_gap);

  if (snapshot.id == UI_RETAINED_ROOT_ID) {
    YGNodeStyleSetWidth(yoga_node, viewport.width);
    YGNodeStyleSetHeight(yoga_node, viewport.height);
    return;
  }

  set_dimension(yoga_node, style.width, YGNodeStyleSetWidth,
                YGNodeStyleSetWidthPercent, YGNodeStyleSetWidthAuto);
  set_dimension(yoga_node, style.height, YGNodeStyleSetHeight,
                YGNodeStyleSetHeightPercent, YGNodeStyleSetHeightAuto);
  set_optional_length(yoga_node, style.min_width, YGNodeStyleSetMinWidth,
                      YGNodeStyleSetMinWidthPercent);
  set_optional_length(yoga_node, style.min_height, YGNodeStyleSetMinHeight,
                      YGNodeStyleSetMinHeightPercent);
  set_optional_length(yoga_node, style.max_width, YGNodeStyleSetMaxWidth,
                      YGNodeStyleSetMaxWidthPercent);
  set_optional_length(yoga_node, style.max_height, YGNodeStyleSetMaxHeight,
                      YGNodeStyleSetMaxHeightPercent);
  set_flex_basis(yoga_node, style.flex_basis);
  if (style.flex_grow > 0.0f)
    YGNodeStyleSetFlexGrow(yoga_node, style.flex_grow);
  if (style.flex_shrink > 0.0f)
    YGNodeStyleSetFlexShrink(yoga_node, style.flex_shrink);
  if (style.aspect_ratio > 0.0f)
    YGNodeStyleSetAspectRatio(yoga_node, style.aspect_ratio);
}

YGNodeRef build_yoga_tree(UiTree &tree, BuildContext &context, NodeId id,
                          LayoutViewport viewport) {
  NodeSnapshot snapshot = {};
  if (!tree.snapshot(id, &snapshot))
    return nullptr;

  YGNodeRef yoga_node = YGNodeNew();
  if (!yoga_node)
    return nullptr;

  apply_style(yoga_node, snapshot, viewport);
  if (snapshot.has_measure) {
    if (context.measure_context_count >= UI_RETAINED_MAX_NODES) {
      YGNodeFree(yoga_node);
      return nullptr;
    }
    MeasureContext &measure_context =
        context.measure_contexts[context.measure_context_count++];
    measure_context = {
        .tree = &tree,
        .id = id,
    };
    YGNodeSetContext(yoga_node, &measure_context);
    YGNodeSetMeasureFunc(yoga_node, measure_yoga_node);
  }

  for (int i = 0; i < tree.child_count(id); ++i) {
    YGNodeRef child =
        build_yoga_tree(tree, context, tree.child_at(id, i), viewport);
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
  BuildContext context = {};
  YGNodeRef root = build_yoga_tree(tree, context, root_id, viewport);
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
