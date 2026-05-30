#pragma once

// Loadout feature-local design tokens (shooter::loadout). Holds the loadout-only
// colors and geometry that no shared component consumes — co-location wins for
// screen-local-only values. Shared colors (title/summary/slots-heading text) and
// the three visual builders come from the shared shooter::tokens palette, which
// this header re-exports via #include. Tokens-only by design: no components.
//
// Colors are STRAIGHT alpha; the IR premultiplies at emit.

#include "client/ui/components/tokens.h"

#include <cstdint>

namespace shooter::loadout {

// ---- Surfaces (loadout-only) ----
// Root Dialog background (loadout_screen.cpp root Dialog .visual).
constexpr ::ui::Color kRootBg = {12, 20, 24, 245};
// "details" column background (the Sunken details panel fill).
constexpr ::ui::Color kDetailsBg = {22, 30, 36, 255};

// ---- Frame geometry (root Dialog) ----
constexpr float kFramePadding = 24.0f;
constexpr float kFrameGap = 18.0f;

// ---- Title geometry ----
constexpr float kTitleHeight = 30.0f;

// ---- Tabs row geometry ----
constexpr float kTabsGap = 10.0f;
// Single tab item (adapts ui Button).
constexpr float kTabWidth = 132.0f;
constexpr float kTabHeight = 34.0f;
constexpr float kTabPadX = 12.0f;
constexpr float kTabPadY = 7.0f;

// ---- Body row geometry ----
constexpr float kBodyGap = 18.0f;

// ---- Weapon grid geometry ----
constexpr float kGridWidth = 400.0f;
constexpr float kGridGap = 10.0f;
constexpr float kGridRowGap = 10.0f;

// ---- Details column geometry ----
constexpr float kDetailsWidth = 260.0f;
constexpr float kDetailsPadding = 14.0f;
constexpr float kDetailsGap = 10.0f;

} // namespace shooter::loadout
