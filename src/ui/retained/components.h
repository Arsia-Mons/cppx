#pragma once

#include "../../react.h"
#include "ui_tree.h"

#include <functional>

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
    bool initial_focus = false;
    bool modal = false;
    Color background = {};
    Color border = {};
    Color text_color = {};
    float border_width = 0.0f;
    uint16_t font_size = 0;
};

struct ButtonProps {
    const char *key = nullptr;
    const char *id = nullptr;
    int offset = 0;
    const char *label = nullptr;
    bool disabled = false;
    bool initial_focus = false;
    Length width = Length::points(132.0f);
    Length height = Length::points(38.0f);
    std::function<void()> on_focus = {};
    std::function<void()> on_confirm = {};
};

struct ToggleProps {
    const char *key = nullptr;
    const char *id = nullptr;
    int offset = 0;
    const char *label = nullptr;
    bool checked = false;
    bool disabled = false;
    bool initial_focus = false;
    Length width = Length::points(178.0f);
    Length height = Length::points(38.0f);
    std::function<void()> on_focus = {};
    std::function<void(bool)> on_change = {};
};

struct SelectableProps {
    const char *key = nullptr;
    const char *id = nullptr;
    int offset = 0;
    const char *label = nullptr;
    bool selected = false;
    bool disabled = false;
    bool initial_focus = false;
    Length width = Length::points(132.0f);
    Length height = Length::points(34.0f);
    FlexDirection direction = FlexDirection::Column;
    AlignItems align_items = AlignItems::Center;
    JustifyContent justify_content = JustifyContent::Center;
    EdgeSizes padding = {12.0f, 12.0f, 7.0f, 7.0f};
    float gap = 0.0f;
    Color background = {};
    Color border = {};
    Color text_color = {};
    float border_width = 0.0f;
    uint16_t font_size = 0;
    std::function<void()> on_focus = {};
    std::function<void()> on_confirm = {};
};

struct FocusableProps {
    const char *key = nullptr;
    const char *id = nullptr;
    int offset = 0;
    bool disabled = false;
    bool initial_focus = false;
    Length width = Length::auto_size();
    Length height = Length::auto_size();
    float flex_grow = 0.0f;
    FlexDirection direction = FlexDirection::Column;
    AlignItems align_items = AlignItems::Stretch;
    JustifyContent justify_content = JustifyContent::Start;
    EdgeSizes padding = {};
    float gap = 0.0f;
    Color background = {};
    Color border = {};
    float border_width = 0.0f;
    std::function<void()> on_focus = {};
    std::function<void()> on_confirm = {};
};

bool begin_retained_frame(UiTree &tree, float width, float height);
bool end_retained_frame();
bool begin_retained_tree_frame(UiTree &tree, float width, float height);
bool end_retained_tree_frame();
UiTree *current_retained_tree();

Style style_from_props(const NodeProps &props);
Style style_from_props(const SelectableProps &props);
Style style_from_props(const FocusableProps &props);
VisualStyle visual_from_props(const NodeProps &props);
VisualStyle visual_from_props(const SelectableProps &props);
VisualStyle visual_from_props(const FocusableProps &props);

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
void Focusable(const FocusableProps &props);
void ScrollContainer(const NodeProps &props);

template <typename Children>
void Selectable(const SelectableProps &props, Children children) {
    RetainedNodeScope scope("Selectable", props.key ? props.key : props.id,
                            style_from_props(props));
    if (scope.active()) {
        scope.tree()->set_metadata(scope.id(),
                                   {
                                       .role = NodeRole::Selectable,
                                       .control_id = props.id,
                                       .control_offset = props.offset,
                                       .value = props.label,
                                       .interaction =
                                           {
                                               .focusable = true,
                                               .disabled = props.disabled,
                                               .selected = props.selected,
                                               .initial_focus =
                                                   props.initial_focus,
                                           },
                                       .visual = visual_from_props(props),
                                       .on_focus = props.on_focus,
                                       .on_confirm = props.on_confirm,
                                   });
        children();
    }
}

template <typename Children>
void Panel(const NodeProps &props, Children children) {
    RetainedNodeScope scope("Panel", props.key, style_from_props(props));
    if (scope.active()) {
        scope.tree()->set_metadata(scope.id(), {
                                                   .interaction =
                                                       {
                                                           .modal = props.modal,
                                                       },
                                                   .visual =
                                                       visual_from_props(props),
                                               });
    }
    if (scope.active()) {
        children();
    }
}

template <typename Children>
void Focusable(const FocusableProps &props, Children children) {
    RetainedNodeScope scope("Focusable", props.key ? props.key : props.id,
                            style_from_props(props));
    if (scope.active()) {
        scope.tree()->set_metadata(scope.id(),
                                   {
                                       .role = NodeRole::Focusable,
                                       .control_id = props.id,
                                       .control_offset = props.offset,
                                       .interaction =
                                           {
                                               .focusable = true,
                                               .disabled = props.disabled,
                                               .initial_focus =
                                                   props.initial_focus,
                                           },
                                       .visual = visual_from_props(props),
                                       .on_focus = props.on_focus,
                                       .on_confirm = props.on_confirm,
                                   });
        children();
    }
}

template <typename Children>
void ScrollContainer(const NodeProps &props, Children children) {
    RetainedNodeScope scope("ScrollContainer", props.key ? props.key : props.id,
                            style_from_props(props));
    if (scope.active()) {
        scope.tree()->set_metadata(scope.id(), {
                                                   .role =
                                                       NodeRole::ScrollContainer,
                                                   .control_id = props.id,
                                                   .visual =
                                                       visual_from_props(props),
                                               });
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
                                       .visual = visual_from_props(props),
                                   });
        children();
    }
}

} // namespace ui::retained
