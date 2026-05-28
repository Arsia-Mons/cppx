#include "selectable.h"

#include "common.h"

namespace ui::components {
namespace {

retained::Style selectable_style(const SelectableProps &props) {
  return {
      .width = props.width,
      .height = props.height,
      .direction = props.direction,
      .align_items = props.align_items,
      .justify_content = props.justify_content,
      .padding = props.padding,
      .gap = props.gap,
  };
}

retained::VisualStyle selectable_visual(const SelectableProps &props) {
  retained::VisualStyle visual = {
      .background = props.selected ? detail::kSelectableSelectedFill
                                   : detail::kButtonFill,
      .border = detail::kButtonBorder,
      .text = props.text_color,
      .border_width = 1.0f,
      .font_size = props.font_size,
  };
  if (props.background.a > 0)
    visual.background = props.background;
  if (props.border.a > 0)
    visual.border = props.border;
  if (props.border_width > 0.0f)
    visual.border_width = props.border_width;
  return visual;
}

retained::UiElement render_selectable(const SelectableProps &props,
                                      retained::UiElementFrame &frame) {
  return frame.box({
      .key = props.key,
      .style = selectable_style(props),
      .visual = selectable_visual(props),
      .text = {.value = props.label},
      .interaction =
          {
              .focusable = true,
              .disabled = props.disabled,
              .selected = props.selected,
              .initial_focus = props.initial_focus,
          },
      .automation =
          {
              .id = props.id,
              .offset = props.offset,
          },
      .accessibility = {.role = retained::SemanticRole::Button},
      .callbacks =
          {
              .on_focus = props.on_focus,
              .on_confirm = props.on_confirm,
          },
      .children = props.children.count > 0
                      ? props.children
                      : detail::label_child(frame, props.label),
  });
}

} // namespace

retained::UiElement Selectable(retained::UiElementFrame &frame,
                               const SelectableProps &props) {
  return frame.component("Selectable", props, render_selectable,
                         detail::component_key(props.key));
}

} // namespace ui::components
