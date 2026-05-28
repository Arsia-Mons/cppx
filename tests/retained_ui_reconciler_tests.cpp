#include "react.h"
#include "ui/runtime/element.h"
#include "ui/runtime/tree.h"

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

static bool snapshot(UiTree &tree, NodeId id, NodeSnapshot *out) {
  CHECK(tree.snapshot(id, out));
  return true;
}

static bool host_elements_commit_to_retained_tree(void) {
  react_init_runtime();
  UiTree tree;
  UiElementFrame frame;
  UiElementFrameScope frame_scope(frame);

  UiElement root = frame.box({
      .key = "root",
      .style =
          {
              .width = Length::points(300.0f),
              .height = Length::points(120.0f),
              .direction = FlexDirection::Row,
              .gap = 8.0f,
              .background = {10, 20, 30, 255},
              .border = {50, 60, 70, 255},
              .border_width = 2.0f,
          },
      .interaction =
          {
              .focusable = true,
              .initial_focus = true,
          },
      .id = "RootBox",
      .id_offset = 4,
      .accessibility = {.role = SemanticRole::Dialog},
      .children = ::ui::children({
          ::ui::text("Ready", "label"),
      }),
  });

  ReconcileResult result =
      reconcile_retained_tree(tree, frame, root, 640.0f, 480.0f);
  CHECK(result.ok);
  CHECK(tree.child_count(tree.root_id()) == 1);

  NodeId root_box = tree.child_at(tree.root_id(), 0);
  NodeSnapshot root_snapshot = {};
  CHECK(snapshot(tree, root_box, &root_snapshot));
  CHECK(strcmp(root_snapshot.type, "Box") == 0);
  CHECK(strcmp(root_snapshot.key, "root") == 0);
  CHECK(strcmp(root_snapshot.control_id, "RootBox") == 0);
  CHECK(root_snapshot.control_offset == 4);
  CHECK(root_snapshot.role == NodeRole::Box);
  CHECK(root_snapshot.semantic_role == SemanticRole::Dialog);
  CHECK(root_snapshot.interaction.focusable);
  CHECK(root_snapshot.interaction.initial_focus);
  CHECK(root_snapshot.style.direction == FlexDirection::Row);
  CHECK(root_snapshot.style.gap == 8.0f);
  CHECK(root_snapshot.style.background.r == 10);
  CHECK(root_snapshot.child_count == 1);

  NodeSnapshot text_snapshot = {};
  CHECK(snapshot(tree, tree.child_at(root_box, 0), &text_snapshot));
  CHECK(strcmp(text_snapshot.type, "Text") == 0);
  CHECK(strcmp(text_snapshot.key, "label") == 0);
  CHECK(strcmp(text_snapshot.value, "Ready") == 0);
  CHECK(text_snapshot.role == NodeRole::Text);
  CHECK(text_snapshot.has_measure);

  react_shutdown();
  return true;
}

static bool child_lists_flatten_forwarded_children(void) {
  react_init_runtime();
  UiTree tree;
  UiElementFrame frame;
  UiElementFrameScope frame_scope(frame);

  UiChildren forwarded = ::ui::children({
      ::ui::text("Middle", "middle"),
  });
  UiElement root = ::ui::box({
      .key = "root",
      .children = ::ui::children({
          ::ui::text("Before", "before"),
          forwarded,
          ::ui::text("After", "after"),
      }),
  });

  ReconcileResult result =
      reconcile_retained_tree(tree, frame, root, 640.0f, 480.0f);
  CHECK(result.ok);

  NodeId root_box = tree.child_at(tree.root_id(), 0);
  NodeSnapshot root_snapshot = {};
  CHECK(snapshot(tree, root_box, &root_snapshot));
  CHECK(root_snapshot.child_count == 3);

  NodeSnapshot before = {};
  NodeSnapshot middle = {};
  NodeSnapshot after = {};
  CHECK(snapshot(tree, tree.child_at(root_box, 0), &before));
  CHECK(snapshot(tree, tree.child_at(root_box, 1), &middle));
  CHECK(snapshot(tree, tree.child_at(root_box, 2), &after));
  CHECK(strcmp(before.value, "Before") == 0);
  CHECK(strcmp(middle.value, "Middle") == 0);
  CHECK(strcmp(after.value, "After") == 0);

  react_shutdown();
  return true;
}

struct CounterProps {
  const char *key = nullptr;
  const char *prefix = nullptr;
};

static UiElement Counter(const CounterProps &props) {
  int *count = use_state_int(0);
  if (count)
    *count += 1;
  return ::ui::text(use_text_storage("%s:%d", props.prefix, count ? *count : 0),
                    "value");
}

static bool component_elements_own_hook_fiber_entry(void) {
  react_init_runtime();
  UiTree tree;
  UiElementFrame frame;
  UiElementFrameScope frame_scope(frame);

  CounterProps props = {
      .key = "counter",
      .prefix = "seen",
  };

  UiElement first = ::ui::component("Counter", props, Counter, props.key);
  ReconcileResult first_result =
      reconcile_retained_tree(tree, frame, first, 640.0f, 480.0f);
  CHECK(first_result.ok);
  NodeSnapshot first_text = {};
  CHECK(snapshot(tree, tree.child_at(tree.root_id(), 0), &first_text));
  CHECK(strcmp(first_text.value, "seen:1") == 0);

  frame.reset();
  UiElement second = ::ui::component("Counter", props, Counter, props.key);
  ReconcileResult second_result =
      reconcile_retained_tree(tree, frame, second, 640.0f, 480.0f);
  CHECK(second_result.ok);
  NodeSnapshot second_text = {};
  CHECK(snapshot(tree, tree.child_at(tree.root_id(), 0), &second_text));
  CHECK(strcmp(second_text.value, "seen:2") == 0);

  react_shutdown();
  return true;
}

static ReactContext g_label_context = {};

struct LabelProps {
  const char *key = nullptr;
};

static UiElement LabelFromContext(const LabelProps &props) {
  (void)props;
  const char *value = static_cast<const char *>(use_context(&g_label_context));
  return ::ui::text(value ? value : "missing", "label");
}

static bool provider_elements_scope_context_for_children(void) {
  react_init_runtime();
  g_label_context = {};
  UiTree tree;
  UiElementFrame frame;
  UiElementFrameScope frame_scope(frame);
  const char *value = "context-value";

  UiElement root = ::ui::provider(
      "LabelProvider", &g_label_context, const_cast<char *>(value),
      ::ui::children({
          ::ui::component("LabelFromContext", LabelProps{.key = "label"},
                          LabelFromContext, "label"),
      }),
      "provider");

  ReconcileResult result =
      reconcile_retained_tree(tree, frame, root, 640.0f, 480.0f);
  CHECK(result.ok);

  NodeSnapshot text = {};
  CHECK(snapshot(tree, tree.child_at(tree.root_id(), 0), &text));
  CHECK(strcmp(text.value, "context-value") == 0);
  CHECK(g_label_context.current == nullptr);

  react_shutdown();
  return true;
}

int main(void) {
  if (!host_elements_commit_to_retained_tree())
    return 1;
  if (!child_lists_flatten_forwarded_children())
    return 1;
  if (!component_elements_own_hook_fiber_entry())
    return 1;
  if (!provider_elements_scope_context_for_children())
    return 1;
  return 0;
}
