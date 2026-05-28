#include "checkbox.h"

namespace ui::components {
namespace {

retained::UiChildren checkbox_children(retained::UiElementFrame &frame,
                                       const CheckboxProps &props) {
  retained::Style mark_style = {
      .width = retained::Length::points(18.0f),
      .height = retained::Length::points(18.0f),
      .background =
          props.checked ? detail::kCheckboxCheckedFill : retained::Color{},
      .border = detail::kControlBorder,
      .border_width = 1.0f,
  };
  retained::UiElement mark = frame.host(
      retained::HostKind::Box,
      {
          .key = "mark",
          .style = mark_style,
      });
  if (!props.label)
    return frame.children({mark});
  return frame.children({
      mark,
      frame.text(props.label, "label"),
  });
}

retained::UiElement render_checkbox(const CheckboxProps &props,
                                    retained::UiElementFrame &frame) {
  retained::HostCallbacks callbacks =
      detail::callbacks_from_props(props);
  callbacks.on_activate = [checked = props.checked,
                           on_change = props.on_change,
                           original = callbacks.on_activate](
                              const retained::ActivationEvent &event) {
    if (original)
      original(event);
    if (on_change)
      on_change(!checked);
  };

  retained::NodeInteraction interaction =
      detail::interaction_from_props(props, true);
  interaction.checked = props.checked;

  return frame.host(
      retained::HostKind::Checkbox,
      {
          .key = props.key,
          .id = props.id,
          .id_offset = props.id_offset,
          .style = detail::control_style(props.disabled, props.style),
          .text = {.value = props.label},
          .interaction = interaction,
          .accessibility = detail::accessibility_from_props(
              props, retained::SemanticRole::Checkbox),
          .callbacks = callbacks,
          .children = checkbox_children(frame, props),
      });
}

} // namespace

retained::UiElement Checkbox(retained::UiElementFrame &frame,
                             const CheckboxProps &props) {
  return frame.component("Checkbox", props, render_checkbox,
                         detail::component_key(props));
}

} // namespace ui::components
