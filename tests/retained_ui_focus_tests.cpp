#include "ui/components/components.h"
#include "ui/runtime/flex_layout.h"
#include "ui/runtime/focus.h"
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
using namespace ui::components;

struct FocusTree {
  UiTree tree;
  NodeId root = 0;
  NodeId start = 0;
  NodeId disabled = 0;
  NodeId options = 0;
  NodeId modal = 0;
  NodeId modal_confirm = 0;
};

static bool snapshot_node(UiTree &tree, NodeId id, NodeSnapshot *snapshot) {
  CHECK(id != 0);
  CHECK(tree.snapshot(id, snapshot));
  return true;
}

static bool layout_tree(FocusTree *out, bool show_modal = false) {
  react_init_runtime();
  out->tree.reset();
  out->root = 0;
  out->start = 0;
  out->disabled = 0;
  out->options = 0;
  out->modal = 0;
  out->modal_confirm = 0;

  UiElementFrame frame;
  UiElement modal =
      show_modal
          ? Dialog(frame,
                   {
                       .key = "modal",
                       .style =
                           {
                               .width = Length::points(180.0f),
                               .height = Length::points(80.0f),
                               .align_items = AlignItems::Start,
                               .padding = {8.0f, 8.0f, 8.0f, 8.0f},
                           },
                       .children = frame.children({
                           Button(frame,
                                  {
                                      .key = "confirm",
                                      .id = "ConfirmModalButton",
                                      .label = "Confirm",
                                  }),
                       }),
                   })
          : frame.empty();

  UiElement root = Box(frame, {
                                  .key = "root",
                                  .style =
                                      {
                                          .width = Length::points(320.0f),
                                          .height = Length::points(240.0f),
                                          .align_items = AlignItems::Start,
                                          .padding =
                                              {4.0f, 4.0f, 4.0f, 4.0f},
                                          .gap = 8.0f,
                                      },
                                  .children = frame.children({
                                      Button(frame,
                                             {
                                                 .key = "start",
                                                 .id = "StartButton",
                                                 .label = "Start",
                                             }),
                                      Button(frame,
                                             {
                                                 .key = "disabled",
                                                 .id = "DisabledButton",
                                                 .disabled = true,
                                                 .label = "Disabled",
                                             }),
                                      Button(frame,
                                             {
                                                 .key = "options",
                                                 .id = "OptionsButton",
                                                 .label = "Options",
                                             }),
                                      modal,
                                  }),
                              });

  ReconcileResult result =
      reconcile_retained_tree(out->tree, frame, root, 320.0f, 240.0f);
  CHECK(result.ok);

  FlexLayoutAdapter adapter = make_yoga_flex_layout_adapter();
  CHECK(compute_flex_layout(adapter, out->tree, {320.0f, 240.0f}));

  out->root = out->tree.child_at(out->tree.root_id(), 0);
  out->start = out->tree.child_at(out->root, 0);
  out->disabled = out->tree.child_at(out->root, 1);
  out->options = out->tree.child_at(out->root, 2);
  if (show_modal) {
    out->modal = out->tree.child_at(out->root, 3);
    out->modal_confirm = out->tree.child_at(out->modal, 0);
  }
  return true;
}

static bool navigation_uses_retained_layout_and_skips_disabled(void) {
  FocusTree frame = {};
  CHECK(layout_tree(&frame));

  FocusRuntime focus = {};
  focus_init(&focus);

  CHECK(focus_update(&focus, frame.tree, {}));
  CHECK(focus_focused_id(focus) == frame.start);
  CHECK(focus_changed_id(focus) == frame.start);
  CHECK(focus_source(focus) == FocusSource::Programmatic);

  CHECK(focus_update(&focus, frame.tree,
                     {
                         .nav_down = true,
                         .source = FocusSource::Keyboard,
                     }));
  CHECK(focus_focused_id(focus) == frame.options);
  CHECK(focus_changed_id(focus) == frame.options);
  CHECK(focus_source(focus) == FocusSource::Keyboard);

  CHECK(focus_update(&focus, frame.tree,
                     {
                         .nav_up = true,
                         .source = FocusSource::Gamepad,
                     }));
  CHECK(focus_focused_id(focus) == frame.start);
  CHECK(focus_changed_id(focus) == frame.start);
  CHECK(focus_source(focus) == FocusSource::Gamepad);
  return true;
}

static bool pointer_release_confirms_original_retained_target(void) {
  FocusTree frame = {};
  CHECK(layout_tree(&frame));

  NodeSnapshot options = {};
  CHECK(snapshot_node(frame.tree, frame.options, &options));
  float x = options.layout.x + options.layout.width * 0.5f;
  float y = options.layout.y + options.layout.height * 0.5f;

  FocusRuntime focus = {};
  focus_init(&focus);
  CHECK(focus_update(&focus, frame.tree, {}));
  CHECK(focus_changed_id(focus) == frame.start);
  CHECK(focus_update(&focus, frame.tree, {}));
  CHECK(focus_changed_id(focus) == 0);

  CHECK(focus_update(&focus, frame.tree,
                     {
                         .pointer_pressed = true,
                         .pointer_down = true,
                         .pointer_valid = true,
                         .pointer_x = x,
                         .pointer_y = y,
                         .source = FocusSource::Mouse,
                     }));
  CHECK(focus_focused_id(focus) == frame.options);
  CHECK(focus_confirmed_id(focus) == 0);
  CHECK(focus_source(focus) == FocusSource::Mouse);

  CHECK(focus_update(&focus, frame.tree,
                     {
                         .pointer_released = true,
                         .pointer_valid = true,
                         .pointer_x = x,
                         .pointer_y = y,
                         .source = FocusSource::Mouse,
                     }));
  CHECK(focus_confirmed_id(focus) == frame.options);
  return true;
}

static bool modal_node_traps_focus_to_modal_subtree(void) {
  FocusTree base = {};
  CHECK(layout_tree(&base));

  FocusRuntime focus = {};
  focus_init(&focus);
  CHECK(focus_update(&focus, base.tree, {}));
  CHECK(focus_focused_id(focus) == base.start);
  CHECK(focus_update(&focus, base.tree,
                     {
                         .nav_down = true,
                         .source = FocusSource::Keyboard,
                     }));
  CHECK(focus_focused_id(focus) == base.options);

  FocusTree modal = {};
  CHECK(layout_tree(&modal, true));
  CHECK(focus_update(&focus, modal.tree, {}));
  CHECK(focus.active_scope_id == modal.modal);
  CHECK(focus_focused_id(focus) == modal.modal_confirm);

  CHECK(focus_update(&focus, modal.tree,
                     {
                         .confirm_pressed = true,
                     }));
  CHECK(focus_confirmed_id(focus) == modal.modal_confirm);

  CHECK(focus_update(&focus, base.tree, {}));
  CHECK(focus.active_scope_id == base.tree.root_id());
  CHECK(focus_focused_id(focus) == base.options);
  CHECK(focus_changed_id(focus) == base.options);
  return true;
}

int main(void) {
  if (!navigation_uses_retained_layout_and_skips_disabled())
    return 1;
  if (!pointer_release_confirms_original_retained_target())
    return 1;
  if (!modal_node_traps_focus_to_modal_subtree())
    return 1;
  return 0;
}
