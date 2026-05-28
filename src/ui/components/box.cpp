#include "box.h"

#include "common.h"

namespace ui::components {
namespace {

retained::Style box_style(const BoxProps &props) {
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

retained::VisualStyle box_visual(const BoxProps &props) {
  return {
      .background = props.background,
      .border = props.border,
      .text = props.text_color,
      .border_width = props.border_width,
      .font_size = props.font_size,
  };
}

retained::UiElement render_box(const BoxProps &props,
                               retained::UiElementFrame &frame) {
  return frame.box({
      .key = props.key,
      .style = box_style(props),
      .visual = box_visual(props),
      .interaction = {.modal = props.modal},
      .children = props.children,
  });
}

} // namespace

retained::UiElement Box(retained::UiElementFrame &frame,
                        const BoxProps &props) {
  return frame.component("Box", props, render_box,
                         detail::component_key(props.key));
}

} // namespace ui::components
