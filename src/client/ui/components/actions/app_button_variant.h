#pragma once

// AppButton design-system enums + impl-only layout/variant helpers.
//
// Host detail (::ui::Style / ::ui::StylePatch) is allowed in this component-impl
// header because it is consumed only by app_button.cppx and the leaf components
// that adapt ui::components::Button directly. Screen authors see only the
// semantic AppButtonVariant / AppButtonSize enums on AppButtonProps.
//
// BASELINE (byte-exact): AppButton reproduces the CURRENT ui Button DEFAULT
// geometry, which has NO border. `selected` does NOT change AppButton layout at
// baseline (the selection-border belongs to weapon tiles / equipment slots,
// which are authored later and adapt Button directly). The `selected` param is
// kept on the API for forward-compat but is intentionally ignored here.

#include "ui/components/common.h" // ::ui::Style, ::ui::StylePatch
#include "ui/style/style_patch.h" // ::ui::patch()
#include "client/ui/components/tokens.h" // shooter::tokens accent/danger

namespace shooter {

enum class AppButtonVariant { Primary, Secondary, Danger, Ghost };
enum class AppButtonSize { Md, Sm };

// Layout-only baseline geometry. Mirrors ui::components::ButtonProps default
// Style (button.hx:23-29) for Md; Sm shrinks height/padding to the loadout tab
// metrics. border_width stays 0 (Button's own resolved chrome owns paint).
inline ::ui::Style app_button_layout(AppButtonSize size, bool /*selected*/) {
  switch (size) {
  case AppButtonSize::Sm:
    return {
        .align_items = ::ui::AlignItems::Center,
        .justify_content = ::ui::JustifyContent::Center,
        .width = ::ui::Length::points(132.0f),
        .height = ::ui::Length::points(34.0f),
        .padding = {12.0f, 12.0f, 7.0f, 7.0f},
    };
  case AppButtonSize::Md:
  default:
    return {
        .align_items = ::ui::AlignItems::Center,
        .justify_content = ::ui::JustifyContent::Center,
        .width = ::ui::Length::points(132.0f),
        .height = ::ui::Length::points(38.0f),
        .padding = {14.0f, 14.0f, 8.0f, 8.0f},
    };
  }
}

// Variant paint overlay over the theme's slate Button base. A StylePatch
// overrides base/variant only; the theme's hover/pressed/disabled deltas still
// layer on top (intended). Each fill sets a FLAT 2-stop gradient (top==bottom)
// so the slate base gradient does not bleed through the new solid fill.
//
// CONSEQUENCE of base/variant-only override: on hover/press/disabled the
// theme's slate interaction deltas re-apply on top, so a hovered Ghost reverts
// to the slate hover chrome (not transparent), and Primary/Danger fills are
// tinted by the slate hover gradient. Keeping a variant transparent/branded
// across interaction states would need a per-variant RoleStyle — deferred (no
// screen uses Ghost/Danger yet).
//
// Secondary returns {} on purpose: the empty patch IS the theme default slate
// button, so a plain AppButton with no variant work needed paints unchanged.
inline ::ui::StylePatch app_button_variant_patch(AppButtonVariant variant) {
  switch (variant) {
  case AppButtonVariant::Primary: {
    const ::ui::Color fill = tokens::kAccent;
    return ::ui::patch()
        .background(fill)
        .gradient(::ui::Gradient{
            .angle_deg = 0.0f,
            .stop_count = 2,
            .stops = {{0.0f, fill}, {1.0f, fill}}})
        .border(::ui::Border{
            {1.0f, 1.0f, 1.0f, 1.0f},
            {tokens::kAccentBorder, tokens::kAccentBorder,
             tokens::kAccentBorder, tokens::kAccentBorder}});
  }
  case AppButtonVariant::Danger: {
    const ::ui::Color fill = tokens::kDanger;
    return ::ui::patch()
        .background(fill)
        .gradient(::ui::Gradient{
            .angle_deg = 0.0f,
            .stop_count = 2,
            .stops = {{0.0f, fill}, {1.0f, fill}}})
        .border(::ui::Border{
            {1.0f, 1.0f, 1.0f, 1.0f},
            {tokens::kDangerBorder, tokens::kDangerBorder,
             tokens::kDangerBorder, tokens::kDangerBorder}});
  }
  case AppButtonVariant::Ghost: {
    const ::ui::Color transparent = {0, 0, 0, 0};
    return ::ui::patch()
        .background(transparent)
        .gradient(::ui::Gradient{
            .angle_deg = 0.0f,
            .stop_count = 2,
            .stops = {{0.0f, transparent}, {1.0f, transparent}}})
        .border(::ui::Border{
            {0.0f, 0.0f, 0.0f, 0.0f},
            {transparent, transparent, transparent, transparent}});
  }
  case AppButtonVariant::Secondary:
  default:
    return {};
  }
}

} // namespace shooter
