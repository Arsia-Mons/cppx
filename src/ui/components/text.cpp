#include "text.h"

namespace ui::components {
namespace {

::ui::UiElement render_text(const TextProps &props) {
  return ::ui::host(::ui::HostKind::Text,
                    {
                        .key = props.key,
                        .id = props.id,
                        .id_offset = props.id_offset,
                        .style = props.style,
                        .text = {.value = props.value},
                        .interaction = detail::interaction_from_props(props),
                        .accessibility = detail::accessibility_from_props(
                            props, ::ui::SemanticRole::Auto),
                        .callbacks = detail::callbacks_from_props(props),
                    });
}

} // namespace

const ::ui::Component<TextProps> Text{"Text", render_text};

} // namespace ui::components
