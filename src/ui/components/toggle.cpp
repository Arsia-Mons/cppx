#include "toggle.h"

#include "common.h"

namespace ui::components {
namespace {

retained::Style toggle_style(retained::Length width, retained::Length height) {
  return {
      .width = width,
      .height = height,
      .direction = retained::FlexDirection::Row,
      .align_items = retained::AlignItems::Center,
      .justify_content = retained::JustifyContent::Start,
      .padding = {10.0f, 10.0f, 8.0f, 8.0f},
      .gap = 10.0f,
  };
}

retained::UiChildren toggle_children(retained::UiElementFrame &frame,
                                     const ToggleProps &props,
                                     retained::VisualStyle mark_visual) {
  retained::UiElement mark = frame.box({
      .key = "mark",
      .style =
          {
              .width = retained::Length::points(18.0f),
              .height = retained::Length::points(18.0f),
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

retained::UiElement render_toggle(const ToggleProps &props,
                                  retained::UiElementFrame &frame) {
  retained::VisualStyle mark_visual = {
      .background =
          props.checked ? detail::kToggleCheckedFill : retained::Color{},
      .border = detail::kButtonBorder,
      .border_width = 1.0f,
  };
  return frame.box({
      .key = props.key,
      .style = toggle_style(props.width, props.height),
      .visual = detail::control_visual(props.disabled),
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
      .accessibility = {.role = retained::SemanticRole::Switch},
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

} // namespace

retained::UiElement Toggle(retained::UiElementFrame &frame,
                           const ToggleProps &props) {
  return frame.component("Toggle", props, render_toggle,
                         detail::component_key(props.key));
}

} // namespace ui::components
