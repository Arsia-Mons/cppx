#pragma once

#include "../runtime/element.h"

#include <functional>

namespace ui::components {

struct AccessibilityProps {
  retained::SemanticRole role = retained::SemanticRole::Auto;
  const char *label = nullptr;
  const char *description = nullptr;
};

namespace detail {

struct HostProps {
  retained::HostKind kind = retained::HostKind::Box;
  const char *key = nullptr;
  const char *id = nullptr;
  int id_offset = 0;
  retained::Style style = {};
  retained::TextProps text = {};
  retained::NodeInteraction interaction = {};
  retained::TextEditMetadata text_edit = {};
  retained::AccessibilityProps accessibility = {};
  retained::HostCallbacks callbacks = {};
  retained::UiChildren children = {};
};

inline retained::UiElement Host(retained::UiElementFrame &frame,
                                const HostProps &props) {
  return frame.host(props.kind,
                    {
                        .key = props.key,
                        .id = props.id,
                        .id_offset = props.id_offset,
                        .style = props.style,
                        .text = props.text,
                        .interaction = props.interaction,
                        .text_edit = props.text_edit,
                        .accessibility = props.accessibility,
                        .callbacks = props.callbacks,
                        .children = props.children,
                    });
}

constexpr retained::Color kControlFill = {24, 28, 36, 255};
constexpr retained::Color kControlDisabledFill = {30, 34, 42, 255};
constexpr retained::Color kControlBorder = {78, 88, 104, 255};
constexpr retained::Color kControlDisabledBorder = {62, 68, 78, 255};
constexpr retained::Color kCheckboxCheckedFill = {44, 92, 128, 255};

template <typename Props> inline const char *component_key(const Props &props) {
  return props.key && props.key[0] != '\0' ? props.key : nullptr;
}

template <typename Props>
inline retained::NodeInteraction
interaction_from_props(const Props &props, bool default_focusable = false) {
  return {
      .focusable = default_focusable || props.focusable,
      .disabled = props.disabled,
      .initial_focus = props.autofocus,
  };
}

template <typename Props>
inline retained::AccessibilityProps
accessibility_from_props(const Props &props,
                         retained::SemanticRole fallback_role) {
  retained::SemanticRole role = props.accessibility.role;
  if (role == retained::SemanticRole::Auto)
    role = fallback_role;
  return {
      .role = role,
      .label = props.accessibility.label,
      .description = props.accessibility.description,
  };
}

template <typename Props>
inline retained::HostCallbacks callbacks_from_props(const Props &props) {
  return {
      .on_focus = props.on_focus,
      .on_blur = props.on_blur,
      .on_activate = props.on_activate,
      .on_key = props.on_key,
      .on_text_input = props.on_text_input,
      .on_text_editing = props.on_text_editing,
  };
}

inline retained::Style control_style(bool disabled,
                                     retained::Style style = {}) {
  if (style.background.a == 0)
    style.background = disabled ? kControlDisabledFill : kControlFill;
  if (style.border.a == 0)
    style.border = disabled ? kControlDisabledBorder : kControlBorder;
  if (style.border_width <= 0.0f)
    style.border_width = 1.0f;
  return style;
}

inline retained::UiChildren label_child(retained::UiElementFrame &frame,
                                        const char *label,
                                        retained::Style style = {}) {
  if (!label)
    return {};
  return frame.children({
      frame.text(label, "label", style),
  });
}

} // namespace detail

} // namespace ui::components
