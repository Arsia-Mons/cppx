#include "text.h"

#include "common.h"

namespace ui::components {
namespace {

retained::Style text_style(const TextProps &props) {
  return {
      .width = props.width,
      .height = props.height,
  };
}

retained::VisualStyle text_visual(const TextProps &props) {
  return {
      .text = props.text_color,
      .font_size = props.font_size,
  };
}

retained::UiElement render_text(const TextProps &props,
                                retained::UiElementFrame &frame) {
  return frame.host(retained::HostKind::Text,
                    {
                        .key = props.key,
                        .style = text_style(props),
                        .visual = text_visual(props),
                        .text = {.value = props.value},
                    });
}

} // namespace

retained::UiElement Text(retained::UiElementFrame &frame,
                         const TextProps &props) {
  return frame.component("Text", props, render_text,
                         detail::component_key(props.key));
}

} // namespace ui::components
