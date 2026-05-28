#include "button.h"

namespace ui::components {
namespace {

retained::HostCallbacks button_callbacks(const ButtonProps &props) {
  retained::HostCallbacks callbacks = detail::callbacks_from_props(props);
  if (props.on_activate) {
    callbacks.on_activate = props.on_activate;
  }
  return callbacks;
}

retained::UiElement render_button(const ButtonProps &props,
                                  retained::UiElementFrame &frame) {
  return frame.host(
      retained::HostKind::Button,
      {
          .key = props.key,
          .id = props.id,
          .id_offset = props.id_offset,
          .style = detail::control_style(props.disabled, props.style),
          .text = {.value = props.label},
          .interaction = detail::interaction_from_props(props, true),
          .accessibility = detail::accessibility_from_props(
              props, retained::SemanticRole::Button),
          .callbacks = button_callbacks(props),
          .children = props.children.count > 0
                          ? props.children
                          : detail::label_child(frame, props.label),
      });
}

} // namespace

retained::UiElement Button(retained::UiElementFrame &frame,
                           const ButtonProps &props) {
  return frame.component("Button", props, render_button,
                         detail::component_key(props));
}

} // namespace ui::components
