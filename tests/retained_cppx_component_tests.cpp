#include "tests/fixtures/retained_cppx/retained_components.h"

#include "react.h"
#include "ui/runtime/element.h"
#include "ui/runtime/flex_layout.h"
#include "ui/runtime/yoga_flex_layout.h"

#include <stdio.h>
#include <string.h>

#define CHECK(expr)                                                            \
  do {                                                                         \
    if (!(expr)) {                                                             \
      fprintf(stderr, "CHECK failed at %s:%d: %s\n", __FILE__, __LINE__,       \
              #expr);                                                          \
      return false;                                                            \
    }                                                                          \
  } while (0)

using namespace ui;

static int g_probe_values[1] = {};

static UiElement render_stateful_probe(const StatefulProbeProps &props) {
  int *value = use_state_int(10);
  if (props.write >= 0)
    *value = props.write;
  if (props.slot >= 0 && props.slot < 1)
    g_probe_values[props.slot] = *value;
  return {};
}

UiElement StatefulProbe(const StatefulProbeProps &props) {
  return ::ui::component("StatefulProbe", props, render_stateful_probe,
                         props.key);
}

static bool snapshot(UiTree &tree, NodeId id, NodeSnapshot *out) {
  CHECK(tree.snapshot(id, out));
  return true;
}

static bool generated_cppx_builds_retained_tree_and_hooks(void) {
  react_init_runtime();
  UiTree tree;
  UiElementFrame frame;

  UiElement first = {};
  {
    UiElementFrameScope frame_scope(frame);
    first = BuildGeneratedRetainedTree(77);
  }
  ReconcileResult first_result =
      reconcile_retained_tree(tree, frame, first, 400.0f, 300.0f);
  CHECK(first_result.ok);
  CHECK(g_probe_values[0] == 77);

  CHECK(compute_flex_layout(make_yoga_flex_layout_adapter(), tree,
                            {400.0f, 300.0f}));

  NodeId panel_id = tree.child_at(tree.root_id(), 0);
  CHECK(panel_id != 0);
  NodeSnapshot panel = {};
  CHECK(snapshot(tree, panel_id, &panel));
  CHECK(strcmp(panel.type, "Box") == 0);
  CHECK(strcmp(panel.key, "generated") == 0);
  CHECK(panel.child_count == 2);
  CHECK(panel.layout.width == 240.0f);
  CHECK(panel.layout.height == 120.0f);

  NodeSnapshot title = {};
  CHECK(snapshot(tree, tree.child_at(panel_id, 0), &title));
  CHECK(strcmp(title.type, "Text") == 0);
  CHECK(strcmp(title.key, "title") == 0);
  CHECK(title.has_measure);
  CHECK(title.layout.width == 72.0f);

  NodeSnapshot button = {};
  CHECK(snapshot(tree, tree.child_at(panel_id, 1), &button));
  CHECK(strcmp(button.type, "Button") == 0);
  CHECK(strcmp(button.key, "confirm") == 0);
  CHECK(button.role == NodeRole::Button);
  CHECK(button.semantic_role == SemanticRole::Button);
  CHECK(button.child_count == 1);

  NodeSnapshot button_text = {};
  CHECK(snapshot(tree, tree.child_at(button.id, 0), &button_text));
  CHECK(strcmp(button_text.type, "Text") == 0);
  CHECK(button_text.has_measure);
  CHECK(button_text.layout.width == 56.0f);

  frame.reset();
  UiElement second = {};
  {
    UiElementFrameScope frame_scope(frame);
    second = BuildGeneratedRetainedTree(-1);
  }
  ReconcileResult second_result =
      reconcile_retained_tree(tree, frame, second, 400.0f, 300.0f);
  CHECK(second_result.ok);
  CHECK(g_probe_values[0] == 77);
  return true;
}

int main(void) {
  if (!generated_cppx_builds_retained_tree_and_hooks())
    return 1;
  react_shutdown();
  return 0;
}
