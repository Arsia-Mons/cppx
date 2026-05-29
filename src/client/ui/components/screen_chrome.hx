#pragma once

#include "ui/components/common.h"

#include <functional>

namespace shooter {

namespace theme {

constexpr ::ui::Color kMenuBackground = {8, 14, 18, 255};
constexpr ::ui::Color kGameBackground = {10, 16, 18, 255};
constexpr ::ui::Color kOverlayBackground = {14, 22, 28, 245};
constexpr ::ui::Color kPanelBackground = {17, 24, 30, 255};
constexpr ::ui::Color kHeroPanelBackground = {18, 27, 32, 245};
constexpr ::ui::Color kPanelBorder = {78, 96, 108, 255};
constexpr ::ui::Color kHeroPanelBorder = {83, 108, 118, 255};
constexpr ::ui::Color kTitleText = {236, 246, 242, 255};
constexpr ::ui::Color kHeroTitleText = {235, 246, 242, 255};
constexpr ::ui::Color kSubtitleText = {154, 177, 184, 255};

// Dense-paint builders: every painted shooter node carries a resolved
// VisualStyle so the renderer reads node.visual directly (no legacy node.style
// paint fallback). Colors are STRAIGHT alpha; the IR premultiplies at emit.

// Solid-fill surface (no border).
inline ::ui::VisualStyle fill_visual(::ui::Color background) {
  ::ui::VisualStyle v{};
  v.background = background;
  return v;
}

// Solid-fill surface with a uniform 1px-style border (width supplied via the
// layout Style.border_width; the color is carried here on all four sides).
inline ::ui::VisualStyle panel_visual(::ui::Color background, ::ui::Color border,
                                      float border_width = 1.0f) {
  ::ui::VisualStyle v{};
  v.background = background;
  v.border.width = {border_width, border_width, border_width, border_width};
  v.border.color = {border, border, border, border};
  return v;
}

// Text paint (color + size). align/wrap/line_height stay defaults.
inline ::ui::VisualStyle text_visual(::ui::Color color, uint16_t font_size) {
  ::ui::VisualStyle v{};
  v.text.color = color;
  v.text.font_size = font_size;
  return v;
}

inline ::ui::Style menu_screen_frame_style() {
  return {
      .width = ::ui::Length::percent(100.0f),
      .height = ::ui::Length::percent(100.0f),
      .direction = ::ui::FlexDirection::Column,
      .align_items = ::ui::AlignItems::Center,
      .justify_content = ::ui::JustifyContent::Center,
      .padding = {36.0f, 36.0f, 36.0f, 36.0f},
  };
}

inline ::ui::Style game_screen_frame_style() {
  return {
      .width = ::ui::Length::percent(100.0f),
      .height = ::ui::Length::percent(100.0f),
      .direction = ::ui::FlexDirection::Column,
      .padding = {24.0f, 24.0f, 24.0f, 24.0f},
      .gap = 18.0f,
  };
}

inline ::ui::Style overlay_screen_frame_style() {
  return {
      .width = ::ui::Length::percent(100.0f),
      .height = ::ui::Length::percent(100.0f),
      .direction = ::ui::FlexDirection::Column,
      .align_items = ::ui::AlignItems::Start,
      .padding = {24.0f, 24.0f, 24.0f, 24.0f},
      .gap = 12.0f,
  };
}

inline ::ui::Style centered_overlay_frame_style() {
  return {
      .width = ::ui::Length::percent(100.0f),
      .height = ::ui::Length::percent(100.0f),
      .direction = ::ui::FlexDirection::Column,
      .align_items = ::ui::AlignItems::Center,
      .justify_content = ::ui::JustifyContent::Center,
  };
}

inline ::ui::Style hero_panel_style() {
  return {
      .width = ::ui::Length::points(340.0f),
      .direction = ::ui::FlexDirection::Column,
      .align_items = ::ui::AlignItems::Center,
      .padding = {24.0f, 24.0f, 24.0f, 24.0f},
      .gap = 14.0f,
      // border_width feeds Yoga layout (reserves the border box); the paint
      // color lives in hero_panel_visual().
      .border_width = 1.0f,
  };
}

inline ::ui::Style overlay_panel_style() {
  return {
      .width = ::ui::Length::points(220.0f),
      .direction = ::ui::FlexDirection::Column,
      .align_items = ::ui::AlignItems::Center,
      .padding = {18.0f, 18.0f, 18.0f, 18.0f},
      .gap = 12.0f,
      .border_width = 1.0f,
  };
}

inline ::ui::Style hero_title_style() {
  return {
      .height = ::ui::Length::points(36.0f),
  };
}

inline ::ui::Style screen_title_style() {
  return {
      .height = ::ui::Length::points(32.0f),
  };
}

inline ::ui::Style dialog_title_style() {
  return {
      .height = ::ui::Length::points(34.0f),
  };
}

inline ::ui::Style subtitle_style() {
  return {
      .height = ::ui::Length::points(22.0f),
  };
}

inline ::ui::Style action_row_style() {
  return {
      .direction = ::ui::FlexDirection::Row,
      .align_items = ::ui::AlignItems::Start,
      .gap = 12.0f,
  };
}

// ---- resolved paint (node.visual) for the chrome surfaces above ----
inline ::ui::VisualStyle menu_screen_frame_visual() {
  return fill_visual(kMenuBackground);
}
inline ::ui::VisualStyle game_screen_frame_visual() {
  return fill_visual(kGameBackground);
}
inline ::ui::VisualStyle overlay_screen_frame_visual() {
  return fill_visual(kOverlayBackground);
}
inline ::ui::VisualStyle hero_panel_visual() {
  return panel_visual(kHeroPanelBackground, kHeroPanelBorder);
}
inline ::ui::VisualStyle overlay_panel_visual() {
  return panel_visual(kPanelBackground, kPanelBorder);
}
inline ::ui::VisualStyle hero_title_visual() {
  return text_visual(kHeroTitleText, 30);
}
inline ::ui::VisualStyle screen_title_visual() {
  return text_visual(kTitleText, 26);
}
inline ::ui::VisualStyle dialog_title_visual() {
  return text_visual(kTitleText, 28);
}
inline ::ui::VisualStyle subtitle_visual() {
  return text_visual(kSubtitleText, 16);
}

} // namespace theme

struct MenuScreenFrameProps {
  const char *key = nullptr;
  ::ui::UiChildren children = {};
};

struct GameScreenFrameProps {
  const char *key = nullptr;
  ::ui::UiChildren children = {};
};

struct OverlayScreenFrameProps {
  const char *key = nullptr;
  ::ui::UiChildren children = {};
};

struct CenteredOverlayFrameProps {
  const char *key = nullptr;
  ::ui::UiChildren children = {};
};

struct HeroPanelProps {
  const char *key = nullptr;
  ::ui::UiChildren children = {};
};

struct OverlayPanelProps {
  const char *key = nullptr;
  ::ui::UiChildren children = {};
};

struct HeroTitleProps {
  const char *key = nullptr;
  const char *value = "";
};

struct ScreenTitleProps {
  const char *key = nullptr;
  const char *value = "";
};

struct DialogTitleProps {
  const char *key = nullptr;
  const char *value = "";
};

struct SubtitleTextProps {
  const char *key = nullptr;
  const char *value = "";
};

struct ActionRowProps {
  const char *key = nullptr;
  ::ui::UiChildren children = {};
};

struct MenuButtonProps {
  const char *key = nullptr;
  const char *id = nullptr;
  int id_offset = 0;
  bool disabled = false;
  bool autofocus = false;
  const char *label = nullptr;
  ::ui::UiChildren children = {};
  std::function<void(const ::ui::ActivationEvent &)> on_activate = {};
};

::ui::UiElement MenuScreenFrame(const MenuScreenFrameProps &props);
::ui::UiElement GameScreenFrame(const GameScreenFrameProps &props);
::ui::UiElement OverlayScreenFrame(const OverlayScreenFrameProps &props);
::ui::UiElement CenteredOverlayFrame(const CenteredOverlayFrameProps &props);
::ui::UiElement HeroPanel(const HeroPanelProps &props);
::ui::UiElement OverlayPanel(const OverlayPanelProps &props);
::ui::UiElement HeroTitle(const HeroTitleProps &props);
::ui::UiElement ScreenTitle(const ScreenTitleProps &props);
::ui::UiElement DialogTitle(const DialogTitleProps &props);
::ui::UiElement SubtitleText(const SubtitleTextProps &props);
::ui::UiElement ActionRow(const ActionRowProps &props);
::ui::UiElement MenuButton(const MenuButtonProps &props);

} // namespace shooter
