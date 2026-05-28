#include "dialog.h"

namespace ui::components {
namespace {

retained::UiElement render_dialog(const DialogProps &props,
                                  retained::UiElementFrame &frame) {
  (void)frame;
  retained::NodeInteraction interaction =
      detail::interaction_from_props(props);
  interaction.modal = props.modal;
  return frame.host(retained::HostKind::Dialog,
                    {
                        .key = props.key,
                        .id = props.id,
                        .id_offset = props.id_offset,
                        .style = props.style,
                        .interaction = interaction,
                        .accessibility = detail::accessibility_from_props(
                            props, retained::SemanticRole::Dialog),
                        .callbacks = detail::callbacks_from_props(props),
                        .children = props.children,
                    });
}

} // namespace

retained::UiElement Dialog(retained::UiElementFrame &frame,
                           const DialogProps &props) {
  return frame.component("Dialog", props, render_dialog,
                         detail::component_key(props));
}

} // namespace ui::components
