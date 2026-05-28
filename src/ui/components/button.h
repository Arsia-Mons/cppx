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
  std::function<void(const ::ui::FocusEvent &)> on_focus = {};
  std::function<void(const ::ui::BlurEvent &)> on_blur = {};
  std::function<void(const ::ui::KeyEvent &)> on_key = {};
  std::function<void(const ::ui::TextInputEvent &)> on_text_input = {};
  std::function<void(const ::ui::TextEditingEvent &)> on_text_editing = {};
  const char *label = nullptr;
  ::ui::UiChildren children = {};
  std::function<void(const ::ui::ActivationEvent &)> on_activate = {};
  ::ui::Style style = {
      .width = ::ui::Length::points(132.0f),
      .height = ::ui::Length::points(38.0f),
      .align_items = ::ui::AlignItems::Center,
      .justify_content = ::ui::JustifyContent::Center,
      .padding = {14.0f, 14.0f, 8.0f, 8.0f},
  };
};

::ui::UiElement Button(const ButtonProps &props);

} // namespace ui::components
