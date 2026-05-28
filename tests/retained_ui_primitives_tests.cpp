#include "ui/retained/element_components.h"
#include "ui/retained/flex_layout.h"
#include "ui/retained/yoga_flex_layout.h"

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

static bool same_text(const char *actual, const char *expected) {
  return strcmp(actual ? actual : "", expected ? expected : "") == 0;
}

static bool snapshot_node(UiTree &tree, NodeId id, NodeSnapshot *snapshot) {
  CHECK(id != 0);
  CHECK(tree.snapshot(id, snapshot));
  return true;
}

static bool commit_root(UiTree &tree, UiElementFrame &frame,
                        const UiElement &root, float width, float height) {
  ReconcileResult result =
      reconcile_retained_tree(tree, frame, root, width, height);
  CHECK(result.ok);
  return true;
}

static bool retained_primitives_write_semantic_metadata_and_layout(void) {
  react_init_runtime();
  UiTree tree;
  UiElementFrame frame;

  UiElement root = BoxElement(
      frame,
      {
          .key = "root",
          .width = Length::points(320.0f),
          .height = Length::points(220.0f),
          .align_items = AlignItems::Start,
          .padding = {4.0f, 4.0f, 4.0f, 4.0f},
          .gap = 6.0f,
          .children = frame.children({
              ButtonElement(frame,
                            {
                                .key = "confirm",
                                .id = "ConfirmButton",
                                .label = "Confirm",
                            }),
              ToggleElement(frame,
                            {
                                .key = "music",
                                .id = "MusicToggle",
                                .label = "Music",
                                .checked = true,
                            }),
              SelectableElement(frame,
                                {
                                    .key = "primary",
                                    .id = "PrimarySlot",
                                    .label = "Rifle",
                                    .selected = true,
                                    .disabled = true,
                                }),
          }),
      });
  CHECK(commit_root(tree, frame, root, 320.0f, 220.0f));

  FlexLayoutAdapter adapter = make_yoga_flex_layout_adapter();
  CHECK(compute_flex_layout(adapter, tree, {320.0f, 220.0f}));

  NodeSnapshot root_snapshot = {};
  CHECK(snapshot_node(tree, tree.child_at(tree.root_id(), 0), &root_snapshot));
  CHECK(same_text(root_snapshot.type, "Box"));
  CHECK(root_snapshot.child_count == 3);
  CHECK(root_snapshot.layout.width == 320.0f);
  CHECK(root_snapshot.layout.height == 220.0f);

  NodeSnapshot button = {};
  CHECK(snapshot_node(tree, tree.child_at(root_snapshot.id, 0), &button));
  CHECK(button.role == NodeRole::Generic);
  CHECK(button.semantic_role == SemanticRole::Button);
  CHECK(same_text(button.control_id, "ConfirmButton"));
  CHECK(same_text(button.value, "Confirm"));
  CHECK(button.interaction.focusable);
  CHECK(!button.interaction.disabled);
  CHECK(button.layout.width == 132.0f);
  CHECK(button.layout.height == 38.0f);
  CHECK(button.child_count == 1);

  NodeSnapshot button_label = {};
  CHECK(snapshot_node(tree, tree.child_at(button.id, 0), &button_label));
  CHECK(button_label.role == NodeRole::Text);
  CHECK(same_text(button_label.value, "Confirm"));
  CHECK(button_label.layout.width == 56.0f);
  CHECK(button_label.layout.height == 16.0f);

  NodeSnapshot toggle = {};
  CHECK(snapshot_node(tree, tree.child_at(root_snapshot.id, 1), &toggle));
  CHECK(toggle.role == NodeRole::Generic);
  CHECK(toggle.semantic_role == SemanticRole::Switch);
  CHECK(same_text(toggle.control_id, "MusicToggle"));
  CHECK(toggle.interaction.focusable);
  CHECK(toggle.interaction.checked);
  CHECK(!toggle.interaction.disabled);
  CHECK(toggle.child_count == 2);
  CHECK(toggle.layout.width == 178.0f);
  CHECK(toggle.layout.height == 38.0f);

  NodeSnapshot toggle_mark = {};
  CHECK(snapshot_node(tree, tree.child_at(toggle.id, 0), &toggle_mark));
  CHECK(same_text(toggle_mark.type, "Box"));
  CHECK(toggle_mark.layout.width == 18.0f);
  CHECK(toggle_mark.layout.height == 18.0f);

  NodeSnapshot toggle_label = {};
  CHECK(snapshot_node(tree, tree.child_at(toggle.id, 1), &toggle_label));
  CHECK(toggle_label.role == NodeRole::Text);
  CHECK(same_text(toggle_label.value, "Music"));

  NodeSnapshot selectable = {};
  CHECK(snapshot_node(tree, tree.child_at(root_snapshot.id, 2), &selectable));
  CHECK(selectable.role == NodeRole::Generic);
  CHECK(selectable.semantic_role == SemanticRole::Button);
  CHECK(same_text(selectable.control_id, "PrimarySlot"));
  CHECK(same_text(selectable.value, "Rifle"));
  CHECK(selectable.interaction.focusable);
  CHECK(selectable.interaction.selected);
  CHECK(selectable.interaction.disabled);
  CHECK(selectable.layout.width == 132.0f);
  CHECK(selectable.layout.height == 34.0f);
  return true;
}

static bool reused_nodes_clear_previous_primitive_metadata(void) {
  react_init_runtime();
  UiTree tree;
  int confirm_count = 0;

  {
    UiElementFrame frame;
    UiElement root = ButtonElement(
        frame,
        {
            .key = "confirm",
            .id = "ConfirmButton",
            .label = "Confirm",
            .disabled = true,
            .on_confirm = [&confirm_count] { confirm_count += 1; },
        });
    CHECK(commit_root(tree, frame, root, 200.0f, 80.0f));
  }

  NodeId button_id = tree.child_at(tree.root_id(), 0);
  NodeSnapshot first = {};
  CHECK(snapshot_node(tree, button_id, &first));
  CHECK(first.interaction.disabled);
  CHECK(same_text(first.value, "Confirm"));
  CHECK(!tree.invoke_confirm(button_id));
  CHECK(confirm_count == 0);

  {
    UiElementFrame frame;
    UiElement root = ButtonElement(frame,
                                   {
                                       .key = "confirm",
                                       .id = "ConfirmButton",
                                   });
    CHECK(commit_root(tree, frame, root, 200.0f, 80.0f));
  }

  NodeSnapshot second = {};
  CHECK(snapshot_node(tree, button_id, &second));
  CHECK(!second.interaction.disabled);
  CHECK(same_text(second.value, ""));
  CHECK(second.child_count == 0);
  CHECK(!tree.invoke_confirm(button_id));
  CHECK(confirm_count == 0);
  return true;
}

static bool retained_button_invokes_confirm_callback(void) {
  react_init_runtime();
  UiTree tree;
  UiElementFrame frame;
  int confirm_count = 0;

  UiElement root = ButtonElement(
      frame,
      {
          .key = "confirm",
          .id = "ConfirmButton",
          .label = "Confirm",
          .on_confirm = [&confirm_count] { confirm_count += 1; },
      });
  CHECK(commit_root(tree, frame, root, 200.0f, 80.0f));

  NodeId button_id = tree.child_at(tree.root_id(), 0);
  CHECK(tree.invoke_confirm(button_id));
  CHECK(confirm_count == 1);
  CHECK(!tree.invoke_confirm(tree.root_id()));
  CHECK(confirm_count == 1);
  return true;
}

static bool retained_toggle_invokes_change_callback(void) {
  react_init_runtime();
  UiTree tree;
  UiElementFrame frame;
  int observed = -1;

  UiElement root =
      ToggleElement(frame,
                    {
                        .key = "music",
                        .id = "MusicToggle",
                        .label = "Music",
                        .checked = true,
                        .on_change =
                            [&observed](bool checked) {
                              observed = checked ? 1 : 0;
                            },
                    });
  CHECK(commit_root(tree, frame, root, 220.0f, 80.0f));

  NodeId toggle_id = tree.child_at(tree.root_id(), 0);
  CHECK(tree.invoke_confirm(toggle_id));
  CHECK(observed == 0);
  return true;
}

static bool retained_selectable_invokes_focus_and_confirm_callbacks(void) {
  react_init_runtime();
  UiTree tree;
  UiElementFrame frame;
  int focus_count = 0;
  int confirm_count = 0;

  UiElement root = SelectableElement(
      frame,
      {
          .key = "slot",
          .id = "PrimarySlot",
          .label = "Primary",
          .on_focus = [&focus_count] { focus_count += 1; },
          .on_confirm = [&confirm_count] { confirm_count += 1; },
      });
  CHECK(commit_root(tree, frame, root, 220.0f, 80.0f));

  NodeId selectable_id = tree.child_at(tree.root_id(), 0);
  CHECK(tree.invoke_focus(selectable_id));
  CHECK(focus_count == 1);
  CHECK(tree.invoke_confirm(selectable_id));
  CHECK(confirm_count == 1);
  return true;
}

static bool retained_container_primitives_write_metadata_and_layout(void) {
  react_init_runtime();
  UiTree tree;
  UiElementFrame frame;
  int focus_count = 0;
  int confirm_count = 0;

  UiElement root = BoxElement(
      frame,
      {
          .key = "root",
          .width = Length::points(260.0f),
          .height = Length::points(180.0f),
          .gap = 4.0f,
          .children = frame.children({
              FocusableElement(
                  frame,
                  {
                      .key = "focusable",
                      .id = "FocusableCard",
                      .width = Length::points(120.0f),
                      .height = Length::points(44.0f),
                      .initial_focus = true,
                      .background = {20, 28, 32, 255},
                      .children = frame.children({
                          TextElement(frame,
                                      {
                                          .key = "label",
                                          .value = "Focusable",
                                      }),
                      }),
                      .on_focus = [&focus_count] { focus_count += 1; },
                      .on_confirm = [&confirm_count] { confirm_count += 1; },
                  }),
              ScrollContainerElement(
                  frame,
                  {
                      .key = "scroll",
                      .id = "ScrollArea",
                      .width = Length::points(200.0f),
                      .height = Length::points(80.0f),
                      .gap = 2.0f,
                      .background = {8, 10, 12, 255},
                      .children = frame.children({
                          TextElement(frame,
                                      {
                                          .key = "row",
                                          .value = "Scrollable row",
                                      }),
                      }),
                  }),
          }),
      });
  CHECK(commit_root(tree, frame, root, 260.0f, 180.0f));

  FlexLayoutAdapter adapter = make_yoga_flex_layout_adapter();
  CHECK(compute_flex_layout(adapter, tree, {260.0f, 180.0f}));

  NodeSnapshot root_snapshot = {};
  CHECK(snapshot_node(tree, tree.child_at(tree.root_id(), 0), &root_snapshot));
  CHECK(root_snapshot.child_count == 2);

  NodeSnapshot focusable = {};
  CHECK(snapshot_node(tree, tree.child_at(root_snapshot.id, 0), &focusable));
  CHECK(focusable.role == NodeRole::Generic);
  CHECK(same_text(focusable.control_id, "FocusableCard"));
  CHECK(focusable.interaction.focusable);
  CHECK(focusable.interaction.initial_focus);
  CHECK(focusable.layout.width == 120.0f);
  CHECK(focusable.layout.height == 44.0f);
  CHECK(focusable.child_count == 1);
  CHECK(tree.invoke_focus(focusable.id));
  CHECK(tree.invoke_confirm(focusable.id));
  CHECK(focus_count == 1);
  CHECK(confirm_count == 1);

  NodeSnapshot scroll = {};
  CHECK(snapshot_node(tree, tree.child_at(root_snapshot.id, 1), &scroll));
  CHECK(scroll.role == NodeRole::Generic);
  CHECK(same_text(scroll.control_id, "ScrollArea"));
  CHECK(!scroll.interaction.focusable);
  CHECK(scroll.layout.width == 200.0f);
  CHECK(scroll.layout.height == 80.0f);
  CHECK(scroll.child_count == 1);
  return true;
}

int main(void) {
  if (!retained_primitives_write_semantic_metadata_and_layout())
    return 1;
  if (!reused_nodes_clear_previous_primitive_metadata())
    return 1;
  if (!retained_button_invokes_confirm_callback())
    return 1;
  if (!retained_toggle_invokes_change_callback())
    return 1;
  if (!retained_selectable_invokes_focus_and_confirm_callbacks())
    return 1;
  if (!retained_container_primitives_write_metadata_and_layout())
    return 1;
  return 0;
}
