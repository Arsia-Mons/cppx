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

// Variant paint overlay. BASELINE: all four variants paint identically (the ui
// Button resolves its full chrome from use_theme().button), so the patch is an
// empty no-op today. This is the single place future per-variant polish edits.
inline ::ui::StylePatch app_button_variant_patch(AppButtonVariant /*variant*/) {
  return {};
}

} // namespace shooter
