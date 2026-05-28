#include "element_components.h"

namespace ui::retained {

namespace {

constexpr Color kButtonFill = {24, 28, 36, 255};
constexpr Color kButtonDisabledFill = {30, 34, 42, 255};
constexpr Color kButtonBorder = {78, 88, 104, 255};
constexpr Color kButtonDisabledBorder = {62, 68, 78, 255};
constexpr Color kToggleCheckedFill = {44, 92, 128, 255};
constexpr Color kSelectableSelectedFill = {42, 80, 60, 255};

const char *component_key(const char *key) {
  return key && key[0] != '\0' ? key : nullptr;
}

Style button_style(Length width, Length height) {
  return {
      .width = width,
      .height = height,
      .direction = FlexDirection::Column,
      .align_items = AlignItems::Center,
      .justify_content = JustifyContent::Center,
      .padding = {14.0f, 14.0f, 8.0f, 8.0f},
  };
}

Style toggle_style(Length width, Length height) {
  return {
      .width = width,
      .height = height,
      .direction = FlexDirection::Row,
      .align_items = AlignItems::Center,
      .justify_content = JustifyContent::Start,
      .padding = {10.0f, 10.0f, 8.0f, 8.0f},
      .gap = 10.0f,
  };
}

Style selectable_style(const ElementSelectableProps &props) {
  return {
      .width = props.width,
      .height = props.height,
      .direction = props.direction,
      .align_items = props.align_items,
      .justify_content = props.justify_content,
      .padding = props.padding,
      .gap = props.gap,
  };
}

Style focusable_style(const ElementFocusableProps &props) {
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

Style scroll_container_style(const ElementScrollContainerProps &props) {
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

VisualStyle control_visual(bool disabled) {
  return {
      .background = disabled ? kButtonDisabledFill : kButtonFill,
      .border = disabled ? kButtonDisabledBorder : kButtonBorder,
      .border_width = 1.0f,
  };
}

VisualStyle selectable_visual(const ElementSelectableProps &props) {
  VisualStyle visual = {
      .background = props.selected ? kSelectableSelectedFill : kButtonFill,
      .border = kButtonBorder,
      .text = props.text_color,
      .border_width = 1.0f,
      .font_size = props.font_size,
  };
  if (props.background.a > 0)
    visual.background = props.background;
  if (props.border.a > 0)
    visual.border = props.border;
  if (props.border_width > 0.0f)
    visual.border_width = props.border_width;
  return visual;
}

UiChildren label_child(UiElementFrame &frame, const char *label) {
  if (!label)
    return {};
  return frame.children({
      frame.text(label, "label"),
  });
}

UiChildren toggle_children(UiElementFrame &frame,
                           const ElementToggleProps &props,
                           VisualStyle mark_visual) {
  UiElement mark = frame.box({
      .key = "mark",
      .style =
          {
              .width = Length::points(18.0f),
              .height = Length::points(18.0f),
          },
      .visual = mark_visual,
  });
  if (!props.label)
    return frame.children({mark});
  return frame.children({
      mark,
      frame.text(props.label, "label"),
  });
}

UiElement render_button(const ElementButtonProps &props,
                        UiElementFrame &frame) {
  return frame.box({
      .key = props.key,
      .style = button_style(props.width, props.height),
      .visual = control_visual(props.disabled),
      .text = {.value = props.label},
      .interaction =
          {
              .focusable = true,
              .disabled = props.disabled,
              .initial_focus = props.initial_focus,
          },
      .automation =
          {
              .id = props.id,
              .offset = props.offset,
          },
      .accessibility = {.role = SemanticRole::Button},
      .callbacks =
          {
              .on_focus = props.on_focus,
              .on_confirm = props.on_confirm,
          },
      .children = label_child(frame, props.label),
  });
}

UiElement render_toggle(const ElementToggleProps &props,
                        UiElementFrame &frame) {
  VisualStyle mark_visual = {
      .background = props.checked ? kToggleCheckedFill : Color{},
      .border = kButtonBorder,
      .border_width = 1.0f,
  };
  return frame.box({
      .key = props.key,
      .style = toggle_style(props.width, props.height),
      .visual = control_visual(props.disabled),
      .text = {.value = props.label},
      .interaction =
          {
              .focusable = true,
              .disabled = props.disabled,
              .checked = props.checked,
              .initial_focus = props.initial_focus,
          },
      .automation =
          {
              .id = props.id,
              .offset = props.offset,
          },
      .accessibility = {.role = SemanticRole::Switch},
      .callbacks =
          {
              .on_focus = props.on_focus,
              .on_confirm =
                  [checked = props.checked, on_change = props.on_change] {
                    if (on_change) {
                      on_change(!checked);
                    }
                  },
          },
      .children = toggle_children(frame, props, mark_visual),
  });
}

UiElement render_selectable(const ElementSelectableProps &props,
                            UiElementFrame &frame) {
  return frame.box({
      .key = props.key,
      .style = selectable_style(props),
      .visual = selectable_visual(props),
      .text = {.value = props.label},
      .interaction =
          {
              .focusable = true,
              .disabled = props.disabled,
              .selected = props.selected,
              .initial_focus = props.initial_focus,
          },
      .automation =
          {
              .id = props.id,
              .offset = props.offset,
          },
      .accessibility = {.role = SemanticRole::Button},
      .callbacks =
          {
              .on_focus = props.on_focus,
              .on_confirm = props.on_confirm,
          },
      .children = props.children.count > 0 ? props.children
                                           : label_child(frame, props.label),
  });
}

UiElement render_focusable(const ElementFocusableProps &props,
                           UiElementFrame &frame) {
  return frame.box({
      .key = props.key,
      .style = focusable_style(props),
      .visual =
          {
              .background = props.background,
              .border = props.border,
              .border_width = props.border_width,
          },
      .interaction =
          {
              .focusable = true,
              .disabled = props.disabled,
              .initial_focus = props.initial_focus,
          },
      .automation =
          {
              .id = props.id,
              .offset = props.offset,
          },
      .callbacks =
          {
              .on_focus = props.on_focus,
              .on_confirm = props.on_confirm,
          },
      .children = props.children,
  });
}

UiElement render_scroll_container(const ElementScrollContainerProps &props,
                                  UiElementFrame &frame) {
  return frame.box({
      .key = props.key,
      .style = scroll_container_style(props),
      .visual =
          {
              .background = props.background,
              .border = props.border,
              .text = props.text_color,
              .border_width = props.border_width,
              .font_size = props.font_size,
          },
      .automation =
          {
              .id = props.id,
          },
      .children = props.children,
  });
}

} // namespace

UiElement ButtonElement(UiElementFrame &frame,
                        const ElementButtonProps &props) {
  return frame.component("Button", props, render_button,
                         component_key(props.key));
}

UiElement ToggleElement(UiElementFrame &frame,
                        const ElementToggleProps &props) {
  return frame.component("Toggle", props, render_toggle,
                         component_key(props.key));
}

UiElement SelectableElement(UiElementFrame &frame,
                            const ElementSelectableProps &props) {
  return frame.component("Selectable", props, render_selectable,
                         component_key(props.key));
}

UiElement FocusableElement(UiElementFrame &frame,
                           const ElementFocusableProps &props) {
  return frame.component("Focusable", props, render_focusable,
                         component_key(props.key));
}

UiElement ScrollContainerElement(UiElementFrame &frame,
                                 const ElementScrollContainerProps &props) {
  return frame.component("ScrollContainer", props, render_scroll_container,
                         component_key(props.key));
}

} // namespace ui::retained
