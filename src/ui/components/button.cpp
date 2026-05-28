#include "button.h"

namespace ui::components {
namespace {

::ui::HostCallbacks button_callbacks(const ButtonProps &props) {
  ::ui::HostCallbacks callbacks = detail::callbacks_from_props(props);
  if (props.on_activate) {
    callbacks.on_activate = props.on_activate;
  }
  return callbacks;
}

} // namespace

::ui::UiElement Button(const ButtonProps &props) {
  return ::ui::host(
      ::ui::HostKind::Button,
      {
          .key = props.key,
          .id = props.id,
          .id_offset = props.id_offset,
          .style = detail::control_style(props.disabled, props.style),
          .text = {.value = props.label},
          .interaction = detail::interaction_from_props(props, true),
          .accessibility = detail::accessibility_from_props(
              props, ::ui::SemanticRole::Button),
          .callbacks = button_callbacks(props),
          .children = props.children.count > 0
                          ? props.children
                          : detail::label_child(props.label),
      });
}

} // namespace ui::components
