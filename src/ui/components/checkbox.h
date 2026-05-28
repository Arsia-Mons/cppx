#pragma once

#include "common.h"

namespace ui::components {

struct CheckboxProps {
  const char *key = nullptr;
  const char *id = nullptr;
  int id_offset = 0;
  bool disabled = false;
  bool focusable = false;
  bool autofocus = false;
  AccessibilityProps accessibility = {};
  std::function<void(const retained::FocusEvent &)> on_focus = {};
  std::function<void(const retained::BlurEvent &)> on_blur = {};
  std::function<void(const retained::ActivationEvent &)> on_activate = {};
  std::function<void(const retained::KeyEvent &)> on_key = {};
  std::function<void(const retained::TextInputEvent &)> on_text_input = {};
  std::function<void(const retained::TextEditingEvent &)> on_text_editing = {};
  bool checked = false;
  const char *label = nullptr;
  std::function<void(bool)> on_change = {};
  retained::Style style = {
      .width = retained::Length::points(178.0f),
      .height = retained::Length::points(38.0f),
      .direction = retained::FlexDirection::Row,
      .align_items = retained::AlignItems::Center,
      .justify_content = retained::JustifyContent::Start,
      .padding = {10.0f, 10.0f, 8.0f, 8.0f},
      .gap = 10.0f,
  };
};

retained::UiElement Checkbox(retained::UiElementFrame &frame,
                             const CheckboxProps &props);

} // namespace ui::components
