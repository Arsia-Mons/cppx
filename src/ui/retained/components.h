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

struct ButtonProps {
    const char *key = nullptr;
    const char *id = nullptr;
    const char *label = nullptr;
    bool disabled = false;
    Length width = Length::points(132.0f);
    Length height = Length::points(38.0f);
};

struct ToggleProps {
    const char *key = nullptr;
    const char *id = nullptr;
    const char *label = nullptr;
    bool checked = false;
    bool disabled = false;
    Length width = Length::points(178.0f);
    Length height = Length::points(38.0f);
};

struct SelectableProps {
    const char *key = nullptr;
    const char *id = nullptr;
    const char *label = nullptr;
    bool selected = false;
    bool disabled = false;
    Length width = Length::points(132.0f);
    Length height = Length::points(34.0f);
};

bool begin_retained_frame(UiTree &tree, float width, float height);
bool end_retained_frame();
bool begin_retained_tree_frame(UiTree &tree, float width, float height);
bool end_retained_tree_frame();
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
void Button(const ButtonProps &props);
void Toggle(const ToggleProps &props);
void Selectable(const SelectableProps &props);

template <typename Children>
void Panel(const NodeProps &props, Children children) {
    RetainedNodeScope scope("Panel", props.key, style_from_props(props));
    if (scope.active()) {
        scope.tree()->set_metadata(scope.id(), {
                                                   .interaction =
                                                       {
                                                           .modal = props.modal,
                                                       },
                                               });
    }
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
        scope.tree()->set_metadata(scope.id(),
                                   {
                                       .role = NodeRole::Button,
                                       .control_id = props.id,
                                       .interaction =
                                           {
                                               .focusable = true,
                                               .disabled = props.disabled,
                                           },
                                   });
        children();
    }
}

} // namespace ui::retained
