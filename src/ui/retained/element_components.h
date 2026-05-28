#pragma once

#include "element.h"

namespace ui::retained {

struct ElementBoxProps {
  const char *key = nullptr;
  Length width = Length::auto_size();
  Length height = Length::auto_size();
  float flex_grow = 0.0f;
  FlexDirection direction = FlexDirection::Column;
  AlignItems align_items = AlignItems::Stretch;
  JustifyContent justify_content = JustifyContent::Start;
  EdgeSizes padding = {};
  float gap = 0.0f;
  bool modal = false;
  Color background = {};
  Color border = {};
  Color text_color = {};
  float border_width = 0.0f;
  uint16_t font_size = 0;
  UiChildren children = {};
};

struct ElementTextProps {
  const char *key = nullptr;
  const char *value = nullptr;
  Length width = Length::auto_size();
  Length height = Length::auto_size();
  Color text_color = {};
  uint16_t font_size = 0;
};

struct ElementButtonProps {
  const char *key = nullptr;
  const char *id = nullptr;
  int offset = 0;
  const char *label = nullptr;
  bool disabled = false;
  bool initial_focus = false;
  Length width = Length::points(132.0f);
  Length height = Length::points(38.0f);
  std::function<void()> on_focus = {};
  std::function<void()> on_confirm = {};
};

struct ElementToggleProps {
  const char *key = nullptr;
  const char *id = nullptr;
  int offset = 0;
  const char *label = nullptr;
  bool checked = false;
  bool disabled = false;
  bool initial_focus = false;
  Length width = Length::points(178.0f);
  Length height = Length::points(38.0f);
  std::function<void()> on_focus = {};
  std::function<void(bool)> on_change = {};
};

struct ElementSelectableProps {
  const char *key = nullptr;
  const char *id = nullptr;
  int offset = 0;
  const char *label = nullptr;
  bool selected = false;
  bool disabled = false;
  bool initial_focus = false;
  Length width = Length::points(132.0f);
  Length height = Length::points(34.0f);
  FlexDirection direction = FlexDirection::Column;
  AlignItems align_items = AlignItems::Center;
  JustifyContent justify_content = JustifyContent::Center;
  EdgeSizes padding = {12.0f, 12.0f, 7.0f, 7.0f};
  float gap = 0.0f;
  Color background = {};
  Color border = {};
  Color text_color = {};
  float border_width = 0.0f;
  uint16_t font_size = 0;
  UiChildren children = {};
  std::function<void()> on_focus = {};
  std::function<void()> on_confirm = {};
};

struct ElementFocusableProps {
  const char *key = nullptr;
  const char *id = nullptr;
  int offset = 0;
  bool disabled = false;
  bool initial_focus = false;
  Length width = Length::auto_size();
  Length height = Length::auto_size();
  float flex_grow = 0.0f;
  FlexDirection direction = FlexDirection::Column;
  AlignItems align_items = AlignItems::Stretch;
  JustifyContent justify_content = JustifyContent::Start;
  EdgeSizes padding = {};
  float gap = 0.0f;
  Color background = {};
  Color border = {};
  float border_width = 0.0f;
  UiChildren children = {};
  std::function<void()> on_focus = {};
  std::function<void()> on_confirm = {};
};

struct ElementScrollContainerProps {
  const char *key = nullptr;
  const char *id = nullptr;
  Length width = Length::auto_size();
  Length height = Length::auto_size();
  float flex_grow = 0.0f;
  FlexDirection direction = FlexDirection::Column;
  AlignItems align_items = AlignItems::Stretch;
  JustifyContent justify_content = JustifyContent::Start;
  EdgeSizes padding = {};
  float gap = 0.0f;
  Color background = {};
  Color border = {};
  Color text_color = {};
  float border_width = 0.0f;
  uint16_t font_size = 0;
  UiChildren children = {};
};

UiElement BoxElement(UiElementFrame &frame, const ElementBoxProps &props);
UiElement TextElement(UiElementFrame &frame, const ElementTextProps &props);
UiElement ButtonElement(UiElementFrame &frame, const ElementButtonProps &props);
UiElement ToggleElement(UiElementFrame &frame, const ElementToggleProps &props);
UiElement SelectableElement(UiElementFrame &frame,
                            const ElementSelectableProps &props);
UiElement FocusableElement(UiElementFrame &frame,
                           const ElementFocusableProps &props);
UiElement ScrollContainerElement(UiElementFrame &frame,
                                 const ElementScrollContainerProps &props);

} // namespace ui::retained
