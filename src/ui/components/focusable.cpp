#include "focusable.h"

#include "common.h"

namespace ui::components {
namespace {

retained::Style focusable_style(const FocusableProps &props) {
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

retained::UiElement render_focusable(const FocusableProps &props,
                                     retained::UiElementFrame &frame) {
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

} // namespace

retained::UiElement Focusable(retained::UiElementFrame &frame,
                              const FocusableProps &props) {
  return frame.component("Focusable", props, render_focusable,
                         detail::component_key(props.key));
}

} // namespace ui::components
