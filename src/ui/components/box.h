#pragma once

#include "common.h"

namespace ui::components {

struct BoxProps {
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
  retained::Style style = {};
  retained::UiChildren children = {};
};

retained::UiElement Box(retained::UiElementFrame &frame, const BoxProps &props);

} // namespace ui::components
