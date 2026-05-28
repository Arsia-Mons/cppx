#pragma once

#include "common.h"

namespace ui::components {

struct ButtonProps {
  const char *key = nullptr;
  const char *id = nullptr;
  int id_offset = 0;
  bool disabled = false;
  bool focusable = false;
  bool autofocus = false;
  AccessibilityProps accessibility = {};
  std::function<void(const retained::FocusEvent &)> on_focus = {};
  std::function<void(const retained::BlurEvent &)> on_blur = {};
  std::function<void(const retained::KeyEvent &)> on_key = {};
  std::function<void(const retained::TextInputEvent &)> on_text_input = {};
  std::function<void(const retained::TextEditingEvent &)> on_text_editing = {};
  const char *label = nullptr;
  retained::UiChildren children = {};
  std::function<void(const retained::ActivationEvent &)> on_activate = {};
  retained::Style style = {
      .width = retained::Length::points(132.0f),
      .height = retained::Length::points(38.0f),
      .align_items = retained::AlignItems::Center,
      .justify_content = retained::JustifyContent::Center,
      .padding = {14.0f, 14.0f, 8.0f, 8.0f},
  };
};

retained::UiElement Button(retained::UiElementFrame &frame,
                           const ButtonProps &props);

} // namespace ui::components
