#include "text.h"

namespace ui::components {
namespace {

retained::UiElement render_text(const TextProps &props,
                                retained::UiElementFrame &frame) {
  return frame.host(retained::HostKind::Text,
                    {
                        .key = props.key,
                        .id = props.id,
                        .id_offset = props.id_offset,
                        .style = props.style,
                        .text = {.value = props.value},
                        .interaction = detail::interaction_from_props(props),
                        .accessibility = detail::accessibility_from_props(
                            props, retained::SemanticRole::Auto),
                        .callbacks = detail::callbacks_from_props(props),
                    });
}

} // namespace

retained::UiElement Text(retained::UiElementFrame &frame,
                         const TextProps &props) {
  return frame.component("Text", props, render_text,
                         detail::component_key(props));
}

} // namespace ui::components
