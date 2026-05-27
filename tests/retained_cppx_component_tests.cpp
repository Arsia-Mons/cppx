#include "tests/fixtures/retained_cppx/retained_components.h"

#include "react.h"
#include "ui/retained/flex_layout.h"
#include "ui/retained/yoga_flex_layout.h"

#include <stdio.h>
#include <string.h>

#define CHECK(expr)                                                            \
    do {                                                                       \
        if (!(expr)) {                                                         \
            fprintf(stderr, "CHECK failed at %s:%d: %s\n", __FILE__, __LINE__, \
                    #expr);                                                    \
            return false;                                                      \
        }                                                                      \
    } while (0)

using namespace ui::retained;

static int g_probe_values[1] = {};

void StatefulProbe(const StatefulProbeProps &props) {
    RetainedNodeScope scope("StatefulProbe", props.key, {});
    if (!scope.active())
        return;
    int *value = use_state_int(10);
    if (props.write >= 0)
        *value = props.write;
    if (props.slot >= 0 && props.slot < 1)
        g_probe_values[props.slot] = *value;
}

static bool snapshot(UiTree &tree, NodeId id, NodeSnapshot *out) {
    CHECK(tree.snapshot(id, out));
    return true;
}

static bool generated_cppx_builds_retained_tree_and_hooks(void) {
    react_init_runtime();
    UiTree tree;

    CHECK(begin_retained_frame(tree, 400.0f, 300.0f));
    BuildGeneratedRetainedTree(77);
    CHECK(end_retained_frame());
    CHECK(g_probe_values[0] == 77);

    CHECK(compute_flex_layout(make_yoga_flex_layout_adapter(), tree,
                              {400.0f, 300.0f}));

    NodeId panel_id = tree.child_at(tree.root_id(), 0);
    CHECK(panel_id != 0);
    NodeSnapshot panel = {};
    CHECK(snapshot(tree, panel_id, &panel));
    CHECK(strcmp(panel.type, "Panel") == 0);
    CHECK(strcmp(panel.key, "generated") == 0);
    CHECK(panel.child_count == 3);
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
    CHECK(button.child_count == 1);

    NodeSnapshot button_text = {};
    CHECK(snapshot(tree, tree.child_at(button.id, 0), &button_text));
    CHECK(strcmp(button_text.type, "Text") == 0);
    CHECK(button_text.has_measure);
    CHECK(button_text.layout.width == 56.0f);

    CHECK(begin_retained_frame(tree, 400.0f, 300.0f));
    BuildGeneratedRetainedTree(-1);
    CHECK(end_retained_frame());
    CHECK(g_probe_values[0] == 77);
    return true;
}

int main(void) {
    if (!generated_cppx_builds_retained_tree_and_hooks())
        return 1;
    react_shutdown();
    return 0;
}
