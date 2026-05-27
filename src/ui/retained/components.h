#pragma once

#include "../../react.h"
#include "ui_tree.h"

namespace ui::retained {

struct NodeProps {
    const char *key = nullptr;
    const char *id = nullptr;
    const char *value = nullptr;
    Length width = Length::auto_size();
    Length height = Length::auto_size();
    float flex_grow = 0.0f;
    FlexDirection direction = FlexDirection::Column;
    AlignItems align_items = AlignItems::Stretch;
    JustifyContent justify_content = JustifyContent::Start;
    EdgeSizes padding = {};
    float gap = 0.0f;
    bool disabled = false;
    bool modal = false;
};

bool begin_retained_frame(UiTree &tree, float width, float height);
bool end_retained_frame();
UiTree *current_retained_tree();

Style style_from_props(const NodeProps &props);

class RetainedNodeScope {
  public:
    RetainedNodeScope(const char *type, const char *key, const Style &style);
    ~RetainedNodeScope();

    RetainedNodeScope(const RetainedNodeScope &) = delete;
    RetainedNodeScope &operator=(const RetainedNodeScope &) = delete;

    bool active() const { return active_; }
    NodeId id() const { return id_; }
    UiTree *tree() const { return tree_; }

  private:
    UiTree *tree_ = nullptr;
    NodeId id_ = 0;
    bool active_ = false;
};

void Panel(const NodeProps &props);
void Text(const NodeProps &props);
void cppx_text(const char *value);

template <typename Children>
void Panel(const NodeProps &props, Children children) {
    RetainedNodeScope scope("Panel", props.key, style_from_props(props));
    if (scope.active()) {
        children();
    }
}

void Button(const NodeProps &props);

template <typename Children>
void Button(const NodeProps &props, Children children) {
    RetainedNodeScope scope("Button", props.key ? props.key : props.id,
                            style_from_props(props));
    if (scope.active()) {
        children();
    }
}

} // namespace ui::retained
