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

inline ::ui::Style menu_screen_frame_style() {
  return {
      .direction = ::ui::FlexDirection::Column,
      .align_items = ::ui::AlignItems::Center,
      .justify_content = ::ui::JustifyContent::Center,
      .width = ::ui::Length::percent(100.0f),
      .height = ::ui::Length::percent(100.0f),
      .padding = {36.0f, 36.0f, 36.0f, 36.0f},
      .background = kMenuBackground,
  };
}

inline ::ui::Style game_screen_frame_style() {
  return {
      .direction = ::ui::FlexDirection::Column,
      .width = ::ui::Length::percent(100.0f),
      .height = ::ui::Length::percent(100.0f),
      .padding = {24.0f, 24.0f, 24.0f, 24.0f},
      .gap = 18.0f,
      .background = kGameBackground,
  };
}

inline ::ui::Style overlay_screen_frame_style() {
  return {
      .direction = ::ui::FlexDirection::Column,
      .align_items = ::ui::AlignItems::Start,
      .width = ::ui::Length::percent(100.0f),
      .height = ::ui::Length::percent(100.0f),
      .padding = {24.0f, 24.0f, 24.0f, 24.0f},
      .gap = 12.0f,
      .background = kOverlayBackground,
  };
}

inline ::ui::Style centered_overlay_frame_style() {
  return {
      .direction = ::ui::FlexDirection::Column,
      .align_items = ::ui::AlignItems::Center,
      .justify_content = ::ui::JustifyContent::Center,
      .width = ::ui::Length::percent(100.0f),
      .height = ::ui::Length::percent(100.0f),
  };
}

inline ::ui::Style hero_panel_style() {
  return {
      .direction = ::ui::FlexDirection::Column,
      .align_items = ::ui::AlignItems::Center,
      .width = ::ui::Length::points(340.0f),
      .padding = {24.0f, 24.0f, 24.0f, 24.0f},
      .gap = 14.0f,
      .background = kHeroPanelBackground,
      .border = kHeroPanelBorder,
      .border_width = 1.0f,
  };
}

inline ::ui::Style overlay_panel_style() {
  return {
      .direction = ::ui::FlexDirection::Column,
      .align_items = ::ui::AlignItems::Center,
      .width = ::ui::Length::points(220.0f),
      .padding = {18.0f, 18.0f, 18.0f, 18.0f},
      .gap = 12.0f,
      .background = kPanelBackground,
      .border = kPanelBorder,
      .border_width = 1.0f,
  };
}

inline ::ui::Style hero_title_style() {
  return {
      .height = ::ui::Length::points(36.0f),
      .text = kHeroTitleText,
      .font_size = 30,
  };
}

inline ::ui::Style screen_title_style() {
  return {
      .height = ::ui::Length::points(32.0f),
      .text = kTitleText,
      .font_size = 26,
  };
}

inline ::ui::Style dialog_title_style() {
  return {
      .height = ::ui::Length::points(34.0f),
      .text = kTitleText,
      .font_size = 28,
  };
}

inline ::ui::Style subtitle_style() {
  return {
      .height = ::ui::Length::points(22.0f),
      .text = kSubtitleText,
      .font_size = 16,
  };
}

inline ::ui::Style action_row_style() {
  return {
      .direction = ::ui::FlexDirection::Row,
      .align_items = ::ui::AlignItems::Start,
      .gap = 12.0f,
  };
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
  std::function<void(const ::ui::ActivationEvent &)> on_activate = {};
  ::ui::UiChildren children = {};
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
