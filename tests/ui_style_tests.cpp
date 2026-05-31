// P0 foundations: VisualStyle paint types, StylePatch + Opt<T> (no sentinels),
// Theme/InteractionState meta, and the MeasureTextFn seam store.
#include "ui/style/interaction.h"
#include "ui/style/style_patch.h"
#include "ui/style/text_measure.h"
#include "ui/style/theme.h"
#include "ui/style/visual_style.h"

#include <stdio.h>
#include <type_traits>

#define CHECK(expr)                                                            \
  do {                                                                         \
    if (!(expr)) {                                                             \
      fprintf(stderr, "CHECK failed at %s:%d: %s\n", __FILE__, __LINE__,       \
              #expr);                                                          \
      return false;                                                            \
    }                                                                          \
  } while (0)

using namespace ui;

// --- compile-time invariants ---
static_assert(std::is_aggregate_v<VisualStyle>);
static_assert(std::is_trivially_copyable_v<VisualStyle>);
static_assert(std::is_aggregate_v<StylePatch>);
static_assert(std::is_trivially_copyable_v<StylePatch>);
static_assert(std::is_trivially_copyable_v<Theme>);
static_assert(std::is_trivially_copyable_v<InteractionState>);
static_assert(std::is_trivially_copyable_v<TextMetricsResult>);

static bool test_opt_presence() {
  Opt<Color> unset;
  CHECK(unset.set == false);
  // The {0,0,0,1} / transparent footgun is unrepresentable: presence is the flag.
  Opt<Color> transparent = opt(Color{0, 0, 0, 0});
  CHECK(transparent.set == true);
  CHECK(transparent.value == (Color{0, 0, 0, 0}));
  return true;
}

static bool test_apply_only_set_fields() {
  VisualStyle vs{};
  vs.corner_radius = 8.f;
  vs.border.width = {1, 1, 1, 1};
  StylePatch p = patch().background({10, 20, 30, 255});
  apply(vs, p);
  // background overridden, everything else untouched:
  CHECK(vs.background == (Color{10, 20, 30, 255}));
  CHECK(vs.corner_radius == 8.f);
  CHECK(vs.border.width.top == 1.f);
  // an explicit transparent override applies (set-flag, not value):
  VisualStyle vs2{};
  vs2.background = {255, 0, 0, 255};
  apply(vs2, patch().background({0, 0, 0, 0}));
  CHECK(vs2.background == (Color{0, 0, 0, 0}));
  return true;
}

static bool test_patch_builder() {
  StylePatch p = patch().background({5, 6, 7, 255}).corner_radius(6.f);
  CHECK(p.background.set && p.corner_radius.set);
  CHECK(p.border.set == false && p.outline.set == false && p.text.set == false);
  return true;
}

static bool test_measurer_store() {
  set_text_measurer(nullptr);
  CHECK(text_measurer() == nullptr);
  return true;
}

static bool test_default_theme() {
  const Theme &t = default_theme();
  // Focus ring is wired ONLY via focus_visible.outline — this is what produces it.
  CHECK(t.button.focus_visible.outline.set);
  CHECK(t.button.focus_visible.outline.value.width > 0.f);
  CHECK(t.input.focus_visible.outline.set);
  CHECK(t.checkbox.focus_visible.outline.set);
  // Hover/press must produce a visible control response; otherwise the
  // interaction hooks can be correct while the UI appears inert. The neutral
  // fallback is FLAT (no gradients) so the response is carried by background.
  CHECK(t.button.hover.background.set);
  CHECK(t.button.hover.border.set);
  CHECK(t.button.pressed.background.set);
  CHECK(t.input.hover.background.set);
  CHECK(t.checkbox.hover.background.set);
  // disabled dims controls:
  CHECK(t.button.disabled.background.set);
  // checkbox mark: invisible until checked.
  CHECK(t.checkbox_mark.base.background.a == 0);
  CHECK(t.checkbox_mark.checked.background.set);
  CHECK(t.checkbox_mark.checked.background.value.a != 0);
  // text default flows through the token + text role base.
  CHECK(t.text.base.text.color == t.text_default);
  CHECK(t.text_default.a != 0);
  return true;
}

int main() {
  bool ok = test_opt_presence() && test_apply_only_set_fields() &&
            test_patch_builder() && test_measurer_store() &&
            test_default_theme();
  if (!ok) {
    fprintf(stderr, "ui_style_tests: FAIL\n");
    return 1;
  }
  printf("ui_style_tests: OK (sizeof VisualStyle=%zu, StylePatch=%zu, Theme=%zu)\n",
         sizeof(VisualStyle), sizeof(StylePatch), sizeof(Theme));
  return 0;
}
