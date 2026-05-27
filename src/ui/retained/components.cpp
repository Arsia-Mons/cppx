#include "components.h"

#include <string.h>

namespace ui::retained {

namespace {

static UiTree *G_current_tree = nullptr;

Size measure_text_node(MeasureInput input, void *user) {
    const char *text = static_cast<const char *>(user);
    float width = text ? (float)strlen(text) * 8.0f : 0.0f;
    if (input.width_mode == MeasureMode::AtMost && width > input.width) {
        width = input.width;
    }
    return {width, 16.0f};
}

} // namespace

bool begin_retained_frame(UiTree &tree, float width, float height) {
    if (G_current_tree)
        return false;
    G_current_tree = &tree;
    tree.begin_frame(width, height);
    react_begin_frame();
    return true;
}

bool end_retained_frame() {
    if (!G_current_tree)
        return false;
    bool ok = G_current_tree->end_frame();
    react_end_frame();
    G_current_tree = nullptr;
    return ok;
}

UiTree *current_retained_tree() { return G_current_tree; }

Style style_from_props(const NodeProps &props) {
    return {
        .width = props.width,
        .height = props.height,
        .flex_grow = props.flex_grow,
        .direction = props.direction,
        .align_items = props.align_items,
        .justify_content = props.justify_content,
        .padding = props.padding,
        .gap = props.gap,
    };
}

RetainedNodeScope::RetainedNodeScope(const char *type, const char *key,
                                     const Style &style) {
    tree_ = current_retained_tree();
    if (!tree_) {
        react_report_error(
            "retained: component entered without active frame\n");
        return;
    }

    uint32_t sibling_index = react_next_child_index();
    bool keyed = key && key[0] != '\0';
    ReactFiberId fiber_id =
        keyed ? react_make_instance_fiber_key_id(type, key)
              : react_make_instance_fiber_id(type, sibling_index, false);
    react_enter(fiber_id);

    id_ = keyed ? tree_->begin_keyed_node(type, key, style)
                : tree_->begin_node(type, style);
    if (!id_) {
        react_leave();
        tree_ = nullptr;
        return;
    }
    active_ = true;
}

RetainedNodeScope::~RetainedNodeScope() {
    if (!active_ || !tree_)
        return;
    tree_->end_node();
    react_leave();
}

void Panel(const NodeProps &props) {
    RetainedNodeScope scope("Panel", props.key, style_from_props(props));
}

void Button(const NodeProps &props) {
    RetainedNodeScope scope("Button", props.key ? props.key : props.id,
                            style_from_props(props));
}

void Text(const NodeProps &props) {
    RetainedNodeScope scope("Text", props.key, style_from_props(props));
    if (!scope.active() || !scope.tree())
        return;
    scope.tree()->set_measure(
        scope.id(), measure_text_node,
        const_cast<char *>(props.value ? props.value : ""));
}

void cppx_text(const char *value) {
    Text({
        .value = value,
    });
}

} // namespace ui::retained
