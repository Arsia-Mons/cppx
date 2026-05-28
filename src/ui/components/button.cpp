#include "button.h"

#include "common.h"

namespace ui::components {
namespace {

retained::Style button_style(retained::Length width, retained::Length height) {
  return {
      .width = width,
      .height = height,
      .direction = retained::FlexDirection::Column,
      .align_items = retained::AlignItems::Center,
      .justify_content = retained::JustifyContent::Center,
      .padding = {14.0f, 14.0f, 8.0f, 8.0f},
  };
}

retained::UiElement render_button(const ButtonProps &props,
                                  retained::UiElementFrame &frame) {
  return frame.box({
      .key = props.key,
      .style = button_style(props.width, props.height),
      .visual = detail::control_visual(props.disabled),
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
      .accessibility = {.role = retained::SemanticRole::Button},
      .callbacks =
          {
              .on_focus = props.on_focus,
              .on_confirm = props.on_confirm,
          },
      .children = detail::label_child(frame, props.label),
  });
}

} // namespace

retained::UiElement Button(retained::UiElementFrame &frame,
                           const ButtonProps &props) {
  return frame.component("Button", props, render_button,
                         detail::component_key(props.key));
}

} // namespace ui::components
