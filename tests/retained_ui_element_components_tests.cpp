#include "react.h"
#include "ui/retained/element.h"
#include "ui/retained/element_components.h"
#include "ui/retained/ui_tree.h"

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

using namespace ui::retained;

static bool snapshot(UiTree &tree, NodeId id, NodeSnapshot *out) {
  CHECK(tree.snapshot(id, out));
  return true;
}

static bool button_element_returns_focusable_box_host(void) {
  react_init_runtime();
  UiTree tree;
  UiElementFrame frame;
  int confirms = 0;

  UiElement root =
      ButtonElement(frame, {
                               .key = "start",
                               .id = "StartButton",
                               .offset = 3,
                               .label = "Start",
                               .initial_focus = true,
                               .on_confirm = [&confirms] { ++confirms; },
                           });

  ReconcileResult result =
      reconcile_retained_tree(tree, frame, root, 640.0f, 480.0f);
  CHECK(result.ok);

  NodeId button = tree.child_at(tree.root_id(), 0);
  NodeSnapshot button_snapshot = {};
  CHECK(snapshot(tree, button, &button_snapshot));
  CHECK(strcmp(button_snapshot.type, "Box") == 0);
  CHECK(strcmp(button_snapshot.key, "start") == 0);
  CHECK(button_snapshot.role == NodeRole::Generic);
  CHECK(button_snapshot.semantic_role == SemanticRole::Button);
  CHECK(button_snapshot.interaction.focusable);
  CHECK(button_snapshot.interaction.initial_focus);
  CHECK(strcmp(button_snapshot.control_id, "StartButton") == 0);
  CHECK(button_snapshot.control_offset == 3);
  CHECK(strcmp(button_snapshot.value, "Start") == 0);
  CHECK(button_snapshot.visual.background.a == 255);
  CHECK(button_snapshot.child_count == 1);

  NodeSnapshot label = {};
  CHECK(snapshot(tree, tree.child_at(button, 0), &label));
  CHECK(strcmp(label.type, "Text") == 0);
  CHECK(strcmp(label.value, "Start") == 0);

  CHECK(tree.invoke_confirm(button));
  CHECK(confirms == 1);

  react_shutdown();
  return true;
}

static bool toggle_element_dispatches_changed_value(void) {
  react_init_runtime();
  UiTree tree;
  UiElementFrame frame;
  bool observed = false;

  UiElement root = ToggleElement(
      frame, {
                 .key = "music",
                 .id = "MusicToggle",
                 .label = "Music",
                 .checked = false,
                 .on_change = [&observed](bool value) { observed = value; },
             });

  ReconcileResult result =
      reconcile_retained_tree(tree, frame, root, 640.0f, 480.0f);
  CHECK(result.ok);

  NodeId toggle = tree.child_at(tree.root_id(), 0);
  NodeSnapshot toggle_snapshot = {};
  CHECK(snapshot(tree, toggle, &toggle_snapshot));
  CHECK(strcmp(toggle_snapshot.type, "Box") == 0);
  CHECK(toggle_snapshot.semantic_role == SemanticRole::Switch);
  CHECK(toggle_snapshot.interaction.focusable);
  CHECK(!toggle_snapshot.interaction.checked);
  CHECK(toggle_snapshot.child_count == 2);

  CHECK(tree.invoke_confirm(toggle));
  CHECK(observed);

  react_shutdown();
  return true;
}

static bool selectable_element_accepts_owned_children(void) {
  react_init_runtime();
  UiTree tree;
  UiElementFrame frame;

  UiElement root =
      SelectableElement(frame, {
                                   .key = "weapon",
                                   .id = "WeaponTile",
                                   .label = "Fallback",
                                   .selected = true,
                                   .children = frame.children({
                                       frame.text("Custom", "custom-label"),
                                   }),
                               });

  ReconcileResult result =
      reconcile_retained_tree(tree, frame, root, 640.0f, 480.0f);
  CHECK(result.ok);

  NodeId selectable = tree.child_at(tree.root_id(), 0);
  NodeSnapshot selectable_snapshot = {};
  CHECK(snapshot(tree, selectable, &selectable_snapshot));
  CHECK(selectable_snapshot.interaction.selected);
  CHECK(selectable_snapshot.child_count == 1);

  NodeSnapshot child = {};
  CHECK(snapshot(tree, tree.child_at(selectable, 0), &child));
  CHECK(strcmp(child.key, "custom-label") == 0);
  CHECK(strcmp(child.value, "Custom") == 0);

  react_shutdown();
  return true;
}

int main(void) {
  if (!button_element_returns_focusable_box_host())
    return 1;
  if (!toggle_element_dispatches_changed_value())
    return 1;
  if (!selectable_element_accepts_owned_children())
    return 1;
  return 0;
}
