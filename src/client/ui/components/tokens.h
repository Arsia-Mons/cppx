#pragma once

// Shared client-UI design tokens (the shadcn "design tokens" analogue for this
// retained UI). One palette + the three generic visual builders that app-authored
// surfaces and text paint resolve through. This replaces the per-screen
// `shooter::theme` namespace that used to live in screen_chrome.hx.
//
// Tokens-only by design: no component exports, no recipe layer. Each semantic
// component owns its variant->token switch locally. Colors are STRAIGHT alpha;
// the IR premultiplies at emit. Header-only (constexpr + inline), not transpiled.

#include "ui/components/common.h"

#include <cstdint>

namespace shooter::tokens {

// ---- Surface backgrounds ----
constexpr ::ui::Color kSurfaceMenu = {8, 14, 18, 255};
constexpr ::ui::Color kSurfaceGame = {10, 16, 18, 255};
constexpr ::ui::Color kSurfaceOverlay = {14, 22, 28, 245};
constexpr ::ui::Color kSurfacePanel = {17, 24, 30, 255};
constexpr ::ui::Color kSurfaceHeroPanel = {18, 27, 32, 245};
constexpr ::ui::Color kSurfaceHudBand = {18, 24, 28, 245};

// ---- Borders ----
constexpr ::ui::Color kBorderPanel = {78, 96, 108, 255};
constexpr ::ui::Color kBorderHeroPanel = {83, 108, 118, 255};
constexpr ::ui::Color kBorderHudBand = {82, 106, 118, 255};

// ---- Text ----
constexpr ::ui::Color kTextTitle = {236, 246, 242, 255};
constexpr ::ui::Color kTextHeroTitle = {235, 246, 242, 255};
constexpr ::ui::Color kTextSubtitle = {154, 177, 184, 255};
constexpr ::ui::Color kTextDialogTitle = {240, 248, 244, 255};
constexpr ::ui::Color kTextBody = {226, 238, 236, 255};
constexpr ::ui::Color kTextBodyMuted = {202, 218, 216, 255};
constexpr ::ui::Color kTextWeaponName = {238, 246, 244, 255};
constexpr ::ui::Color kTextWeaponDetail = {184, 204, 204, 255};
constexpr ::ui::Color kTextWeaponDetailOff = {142, 148, 150, 255};
constexpr ::ui::Color kTextHud = {224, 238, 236, 255};

// ---- Font sizes ----
constexpr uint16_t kFontHeroTitle = 30;
constexpr uint16_t kFontScreenTitle = 26;
constexpr uint16_t kFontDialogTitle = 28;
constexpr uint16_t kFontPopupTitle = 22;
constexpr uint16_t kFontSubtitle = 16;
constexpr uint16_t kFontHud = 18;
constexpr uint16_t kFontStrong = 16;
constexpr uint16_t kFontMessage = 15;
constexpr uint16_t kFontBody = 14;
constexpr uint16_t kFontDetail = 12;

// ---- Border widths ----
constexpr float kBorderWidth = 1.0f;
constexpr float kBorderWidthSelected = 2.0f;

// ---- Visual builders (verbatim semantics from the former screen_chrome theme) ----

// Solid-fill surface (no border).
inline ::ui::VisualStyle fill_visual(::ui::Color background) {
  ::ui::VisualStyle v{};
  v.background = background;
  return v;
}

// Solid-fill surface with a uniform border (width feeds layout via
// Style.border_width; the color is carried here on all four sides).
inline ::ui::VisualStyle panel_visual(::ui::Color background, ::ui::Color border,
                                      float border_width = kBorderWidth) {
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

} // namespace shooter::tokens
