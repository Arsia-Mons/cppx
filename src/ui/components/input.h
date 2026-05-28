#pragma once

#include "common.h"

#include <string>

namespace ui::components {

struct InputProps {
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
  const char *value = "";
  std::function<void(const std::string &)> on_change = {};
  retained::Style style = {
      .width = retained::Length::points(220.0f),
      .height = retained::Length::points(36.0f),
      .align_items = retained::AlignItems::Stretch,
      .justify_content = retained::JustifyContent::Center,
      .padding = {8.0f, 8.0f, 8.0f, 8.0f},
  };
};

retained::UiElement Input(retained::UiElementFrame &frame,
                          const InputProps &props);

} // namespace ui::components
