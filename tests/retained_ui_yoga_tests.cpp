#include "ui/runtime/flex_layout.h"
#include "ui/runtime/tree.h"
#include "ui/runtime/yoga_flex_layout.h"

#include <stdio.h>

#define CHECK(expr)                                                            \
  do {                                                                         \
    if (!(expr)) {                                                             \
      fprintf(stderr, "CHECK failed at %s:%d: %s\n", __FILE__, __LINE__,       \
              #expr);                                                          \
      return false;                                                            \
    }                                                                          \
  } while (0)

using namespace ui::retained;

static bool snapshot(UiTree &tree, NodeId id, NodeSnapshot *out) {
  CHECK(tree.snapshot(id, out));
  return true;
}

static bool yoga_computes_column_gap_and_grow(void) {
  UiTree tree;

  Style panel = {};
  panel.width = Length::points(300.0f);
  panel.height = Length::points(200.0f);
  panel.direction = FlexDirection::Column;
  panel.gap = 10.0f;

  Style fixed = {};
  fixed.height = Length::points(50.0f);

  Style growing = {};
  growing.height = Length::grow(1.0f);

  tree.begin_frame(300.0f, 200.0f);
  NodeId panel_id = tree.begin_keyed_node("Panel", "root", panel);
  NodeId fixed_id = tree.begin_keyed_node("Child", "fixed", fixed);
  CHECK(tree.end_node());
  NodeId growing_id = tree.begin_keyed_node("Child", "growing", growing);
  CHECK(tree.end_node());
  CHECK(tree.end_node());
  CHECK(tree.end_frame());

  CHECK(compute_flex_layout(make_yoga_flex_layout_adapter(), tree,
                            {300.0f, 200.0f}));

  NodeSnapshot panel_snapshot = {};
  NodeSnapshot fixed_snapshot = {};
  NodeSnapshot growing_snapshot = {};
  CHECK(snapshot(tree, panel_id, &panel_snapshot));
  CHECK(snapshot(tree, fixed_id, &fixed_snapshot));
  CHECK(snapshot(tree, growing_id, &growing_snapshot));

  CHECK(panel_snapshot.layout.width == 300.0f);
  CHECK(panel_snapshot.layout.height == 200.0f);
  CHECK(fixed_snapshot.layout.y == 0.0f);
  CHECK(fixed_snapshot.layout.height == 50.0f);
  CHECK(growing_snapshot.layout.y == 60.0f);
  CHECK(growing_snapshot.layout.height == 140.0f);
  return true;
}

static bool yoga_computes_row_percent_and_grow(void) {
  UiTree tree;

  Style row = {};
  row.width = Length::points(400.0f);
  row.height = Length::points(100.0f);
  row.direction = FlexDirection::Row;

  Style half = {};
  half.width = Length::percent(50.0f);

  Style grow = {};
  grow.width = Length::grow(1.0f);

  tree.begin_frame(400.0f, 100.0f);
  NodeId row_id = tree.begin_keyed_node("Row", "root", row);
  NodeId half_id = tree.begin_keyed_node("Child", "half", half);
  CHECK(tree.end_node());
  NodeId grow_id = tree.begin_keyed_node("Child", "grow", grow);
  CHECK(tree.end_node());
  CHECK(tree.end_node());
  CHECK(tree.end_frame());

  CHECK(compute_flex_layout(make_yoga_flex_layout_adapter(), tree,
                            {400.0f, 100.0f}));

  NodeSnapshot row_snapshot = {};
  NodeSnapshot half_snapshot = {};
  NodeSnapshot grow_snapshot = {};
  CHECK(snapshot(tree, row_id, &row_snapshot));
  CHECK(snapshot(tree, half_id, &half_snapshot));
  CHECK(snapshot(tree, grow_id, &grow_snapshot));

  CHECK(row_snapshot.layout.width == 400.0f);
  CHECK(row_snapshot.layout.height == 100.0f);
  CHECK(half_snapshot.layout.x == 0.0f);
  CHECK(half_snapshot.layout.width == 200.0f);
  CHECK(grow_snapshot.layout.x == 200.0f);
  CHECK(grow_snapshot.layout.width == 200.0f);
  return true;
}

static bool yoga_applies_padding_to_child_layout(void) {
  UiTree tree;

  Style panel = {};
  panel.width = Length::points(120.0f);
  panel.height = Length::points(80.0f);
  panel.padding = {8.0f, 4.0f, 6.0f, 2.0f};

  Style child = {};
  child.width = Length::points(20.0f);
  child.height = Length::points(10.0f);

  tree.begin_frame(120.0f, 80.0f);
  NodeId panel_id = tree.begin_keyed_node("Panel", "padded", panel);
  NodeId child_id = tree.begin_keyed_node("Child", "content", child);
  CHECK(tree.end_node());
  CHECK(tree.end_node());
  CHECK(tree.end_frame());

  CHECK(compute_flex_layout(make_yoga_flex_layout_adapter(), tree,
                            {120.0f, 80.0f}));

  NodeSnapshot panel_snapshot = {};
  NodeSnapshot child_snapshot = {};
  CHECK(snapshot(tree, panel_id, &panel_snapshot));
  CHECK(snapshot(tree, child_id, &child_snapshot));

  CHECK(panel_snapshot.layout.width == 120.0f);
  CHECK(panel_snapshot.layout.height == 80.0f);
  CHECK(child_snapshot.layout.x == 8.0f);
  CHECK(child_snapshot.layout.y == 6.0f);
  CHECK(child_snapshot.layout.width == 20.0f);
  CHECK(child_snapshot.layout.height == 10.0f);
  return true;
}

static bool yoga_applies_margins_min_max_and_absolute_positioning(void) {
  UiTree tree;

  Style row = {};
  row.width = Length::points(220.0f);
  row.height = Length::points(90.0f);
  row.direction = FlexDirection::Row;
  row.align_items = AlignItems::Start;

  Style first = {};
  first.width = Length::points(40.0f);
  first.height = Length::points(20.0f);
  first.margin = {10.0f, 5.0f, 0.0f, 0.0f};

  Style clamped = {};
  clamped.width = Length::points(200.0f);
  clamped.height = Length::points(10.0f);
  clamped.max_width = Length::points(90.0f);
  clamped.min_height = Length::points(24.0f);

  Style absolute = {};
  absolute.position = PositionType::Absolute;
  absolute.position_inset = {120.0f, 0.0f, 35.0f, 0.0f};
  absolute.width = Length::points(20.0f);
  absolute.height = Length::points(15.0f);

  tree.begin_frame(220.0f, 90.0f);
  NodeId row_id = tree.begin_keyed_node("Row", "root", row);
  NodeId first_id = tree.begin_keyed_node("Child", "first", first);
  CHECK(tree.end_node());
  NodeId clamped_id = tree.begin_keyed_node("Child", "clamped", clamped);
  CHECK(tree.end_node());
  NodeId absolute_id = tree.begin_keyed_node("Child", "absolute", absolute);
  CHECK(tree.end_node());
  CHECK(tree.end_node());
  CHECK(tree.end_frame());

  CHECK(compute_flex_layout(make_yoga_flex_layout_adapter(), tree,
                            {220.0f, 90.0f}));

  NodeSnapshot row_snapshot = {};
  NodeSnapshot first_snapshot = {};
  NodeSnapshot clamped_snapshot = {};
  NodeSnapshot absolute_snapshot = {};
  CHECK(snapshot(tree, row_id, &row_snapshot));
  CHECK(snapshot(tree, first_id, &first_snapshot));
  CHECK(snapshot(tree, clamped_id, &clamped_snapshot));
  CHECK(snapshot(tree, absolute_id, &absolute_snapshot));

  CHECK(row_snapshot.layout.width == 220.0f);
  CHECK(first_snapshot.layout.x == 10.0f);
  CHECK(first_snapshot.layout.width == 40.0f);
  CHECK(clamped_snapshot.layout.x == 55.0f);
  CHECK(clamped_snapshot.layout.width == 90.0f);
  CHECK(clamped_snapshot.layout.height == 24.0f);
  CHECK(absolute_snapshot.layout.x == 120.0f);
  CHECK(absolute_snapshot.layout.y == 35.0f);
  CHECK(absolute_snapshot.layout.width == 20.0f);
  return true;
}

static bool yoga_applies_wrap_and_row_gap(void) {
  UiTree tree;

  Style row = {};
  row.width = Length::points(100.0f);
  row.height = Length::points(80.0f);
  row.direction = FlexDirection::Row;
  row.wrap = FlexWrap::Wrap;
  row.align_items = AlignItems::Start;
  row.align_content = AlignItems::Start;
  row.row_gap = 6.0f;

  Style child = {};
  child.width = Length::points(60.0f);
  child.height = Length::points(20.0f);

  tree.begin_frame(100.0f, 80.0f);
  NodeId row_id = tree.begin_keyed_node("Row", "wrap", row);
  NodeId first_id = tree.begin_keyed_node("Child", "first", child);
  CHECK(tree.end_node());
  NodeId second_id = tree.begin_keyed_node("Child", "second", child);
  CHECK(tree.end_node());
  CHECK(tree.end_node());
  CHECK(tree.end_frame());

  CHECK(compute_flex_layout(make_yoga_flex_layout_adapter(), tree,
                            {100.0f, 80.0f}));

  NodeSnapshot row_snapshot = {};
  NodeSnapshot first_snapshot = {};
  NodeSnapshot second_snapshot = {};
  CHECK(snapshot(tree, row_id, &row_snapshot));
  CHECK(snapshot(tree, first_id, &first_snapshot));
  CHECK(snapshot(tree, second_id, &second_snapshot));

  CHECK(row_snapshot.layout.width == 100.0f);
  CHECK(first_snapshot.layout.x == 0.0f);
  CHECK(first_snapshot.layout.y == 0.0f);
  CHECK(second_snapshot.layout.x == 0.0f);
  CHECK(second_snapshot.layout.y == 26.0f);
  return true;
}

struct MeasureProbe {
  int count = 0;
  MeasureInput input = {};
};

static Size measure_text_node(MeasureInput input, void *user) {
  MeasureProbe *probe = static_cast<MeasureProbe *>(user);
  if (probe) {
    probe->count += 1;
    probe->input = input;
  }
  return {72.0f, 18.0f};
}

static bool yoga_uses_retained_measure_function(void) {
  UiTree tree;
  MeasureProbe probe = {};

  Style panel = {};
  panel.width = Length::points(160.0f);
  panel.height = Length::points(80.0f);
  panel.align_items = AlignItems::Start;

  tree.begin_frame(160.0f, 80.0f);
  NodeId panel_id = tree.begin_keyed_node("Panel", "text-container", panel);
  NodeId text_id = tree.begin_keyed_node("Text", "label");
  CHECK(tree.set_measure(text_id, measure_text_node, &probe));
  CHECK(tree.end_node());
  CHECK(tree.end_node());
  CHECK(tree.end_frame());

  CHECK(compute_flex_layout(make_yoga_flex_layout_adapter(), tree,
                            {160.0f, 80.0f}));

  NodeSnapshot panel_snapshot = {};
  NodeSnapshot text_snapshot = {};
  CHECK(snapshot(tree, panel_id, &panel_snapshot));
  CHECK(snapshot(tree, text_id, &text_snapshot));

  CHECK(probe.count > 0);
  CHECK(panel_snapshot.layout.width == 160.0f);
  CHECK(text_snapshot.layout.width == 72.0f);
  CHECK(text_snapshot.layout.height == 18.0f);
  return true;
}

int main(void) {
  if (!yoga_computes_column_gap_and_grow())
    return 1;
  if (!yoga_computes_row_percent_and_grow())
    return 1;
  if (!yoga_applies_padding_to_child_layout())
    return 1;
  if (!yoga_applies_margins_min_max_and_absolute_positioning())
    return 1;
  if (!yoga_applies_wrap_and_row_gap())
    return 1;
  if (!yoga_uses_retained_measure_function())
    return 1;
  return 0;
}
