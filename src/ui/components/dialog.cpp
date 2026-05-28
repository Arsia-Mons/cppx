#include "dialog.h"

namespace ui::components {
namespace {

::ui::UiElement render_dialog(const DialogProps &props) {
  ::ui::NodeInteraction interaction = detail::interaction_from_props(props);
  interaction.modal = props.modal;
  return ::ui::host(::ui::HostKind::Dialog,
                    {
                        .key = props.key,
                        .id = props.id,
                        .id_offset = props.id_offset,
                        .style = props.style,
                        .interaction = interaction,
                        .accessibility = detail::accessibility_from_props(
                            props, ::ui::SemanticRole::Dialog),
                        .callbacks = detail::callbacks_from_props(props),
                        .children = props.children,
                    });
}

} // namespace

const ::ui::Component<DialogProps> Dialog{"Dialog", render_dialog};

} // namespace ui::components
