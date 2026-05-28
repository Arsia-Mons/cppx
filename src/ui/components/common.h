#pragma once

#include "../runtime/element.h"

namespace ui::components::detail {

constexpr retained::Color kButtonFill = {24, 28, 36, 255};
constexpr retained::Color kButtonDisabledFill = {30, 34, 42, 255};
constexpr retained::Color kButtonBorder = {78, 88, 104, 255};
constexpr retained::Color kButtonDisabledBorder = {62, 68, 78, 255};
constexpr retained::Color kToggleCheckedFill = {44, 92, 128, 255};
constexpr retained::Color kSelectableSelectedFill = {42, 80, 60, 255};

inline const char *component_key(const char *key) {
  return key && key[0] != '\0' ? key : nullptr;
}

inline retained::VisualStyle control_visual(bool disabled) {
  return {
      .background = disabled ? kButtonDisabledFill : kButtonFill,
      .border = disabled ? kButtonDisabledBorder : kButtonBorder,
      .border_width = 1.0f,
  };
}

inline retained::UiChildren label_child(retained::UiElementFrame &frame,
                                        const char *label) {
  if (!label)
    return {};
  return frame.children({
      frame.text(label, "label"),
  });
}

} // namespace ui::components::detail
