#include "scroll_container.h"

#include "common.h"

namespace ui::components {
namespace {

retained::Style scroll_container_style(const ScrollContainerProps &props) {
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

retained::UiElement render_scroll_container(const ScrollContainerProps &props,
                                            retained::UiElementFrame &frame) {
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

retained::UiElement ScrollContainer(retained::UiElementFrame &frame,
                                    const ScrollContainerProps &props) {
  return frame.component("ScrollContainer", props, render_scroll_container,
                         detail::component_key(props.key));
}

} // namespace ui::components
