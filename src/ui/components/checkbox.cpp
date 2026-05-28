#include "checkbox.h"

namespace ui::components {
namespace {

::ui::UiChildren checkbox_children(const CheckboxProps &props) {
  ::ui::Style mark_style = {
      .width = ::ui::Length::points(18.0f),
      .height = ::ui::Length::points(18.0f),
      .background =
          props.checked ? detail::kCheckboxCheckedFill : ::ui::Color{},
      .border = detail::kControlBorder,
      .border_width = 1.0f,
  };
  ::ui::UiElement mark =
      ::ui::host(::ui::HostKind::Box, {
                                          .key = "mark",
                                          .style = mark_style,
                                      });
  if (!props.label)
    return ::ui::children({mark});
  return ::ui::children({
      mark,
      ::ui::text(props.label, "label"),
  });
}

} // namespace

::ui::UiElement Checkbox(const CheckboxProps &props) {
  ::ui::HostCallbacks callbacks = detail::callbacks_from_props(props);
  callbacks.on_activate =
      [checked = props.checked, on_change = props.on_change,
       original = callbacks.on_activate](const ::ui::ActivationEvent &event) {
        if (original)
          original(event);
        if (on_change)
          on_change(!checked);
      };

  ::ui::NodeInteraction interaction =
      detail::interaction_from_props(props, true);
  interaction.checked = props.checked;

  return ::ui::host(
      ::ui::HostKind::Checkbox,
      {
          .key = props.key,
          .id = props.id,
          .id_offset = props.id_offset,
          .style = detail::control_style(props.disabled, props.style),
          .text = {.value = props.label},
          .interaction = interaction,
          .accessibility = detail::accessibility_from_props(
              props, ::ui::SemanticRole::Checkbox),
          .callbacks = callbacks,
          .children = checkbox_children(props),
      });
}

} // namespace ui::components
