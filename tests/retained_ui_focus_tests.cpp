#include "ui/retained/components.h"
#include "ui/retained/flex_layout.h"
#include "ui/retained/focus.h"
#include "ui/retained/yoga_flex_layout.h"

#include <stdio.h>

#define CHECK(expr)                                                            \
    do {                                                                       \
        if (!(expr)) {                                                         \
            fprintf(stderr, "CHECK failed at %s:%d: %s\n", __FILE__, __LINE__, \
                    #expr);                                                    \
            return false;                                                      \
        }                                                                      \
    } while (0)

using namespace ui::retained;

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

    CHECK(begin_retained_frame(out->tree, 320.0f, 240.0f));
    Panel(
        {
            .key = "root",
            .width = Length::points(320.0f),
            .height = Length::points(240.0f),
            .gap = 8.0f,
            .padding = {4.0f, 4.0f, 4.0f, 4.0f},
            .align_items = AlignItems::Start,
        },
        [show_modal] {
            Button(ButtonProps{
                .key = "start",
                .id = "StartButton",
                .label = "Start",
            });
            Button(ButtonProps{
                .key = "disabled",
                .id = "DisabledButton",
                .label = "Disabled",
                .disabled = true,
            });
            Button(ButtonProps{
                .key = "options",
                .id = "OptionsButton",
                .label = "Options",
            });
            if (show_modal) {
                Panel(
                    {
                        .key = "modal",
                        .width = Length::points(180.0f),
                        .height = Length::points(80.0f),
                        .padding = {8.0f, 8.0f, 8.0f, 8.0f},
                        .align_items = AlignItems::Start,
                        .modal = true,
                    },
                    [] {
                        Button(ButtonProps{
                            .key = "confirm",
                            .id = "ConfirmModalButton",
                            .label = "Confirm",
                        });
                    });
            }
        });
    CHECK(end_retained_frame());

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
    CHECK(focus_source(focus) == FocusSource::Programmatic);

    CHECK(focus_update(&focus, frame.tree,
                       {
                           .nav_down = true,
                           .source = FocusSource::Keyboard,
                       }));
    CHECK(focus_focused_id(focus) == frame.options);
    CHECK(focus_source(focus) == FocusSource::Keyboard);

    CHECK(focus_update(&focus, frame.tree,
                       {
                           .nav_up = true,
                           .source = FocusSource::Gamepad,
                       }));
    CHECK(focus_focused_id(focus) == frame.start);
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
    CHECK(focus_focused_id(focus) == base.start);
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
