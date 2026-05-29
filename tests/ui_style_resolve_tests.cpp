// P1: resolve() cascade — pin the locked precedence (design §5.1, §13.2).
#include "ui/style/resolve.h"

#include <stdio.h>
#include <string.h>

#define CHECK(expr)                                                            \
  do {                                                                         \
    if (!(expr)) {                                                             \
      fprintf(stderr, "CHECK failed at %s:%d: %s\n", __FILE__, __LINE__,       \
              #expr);                                                          \
      return false;                                                            \
    }                                                                          \
  } while (0)

using namespace ui;

static RoleStyle make_role() {
  RoleStyle r{};
  r.base.background = {10, 10, 10, 255};
  r.hover = patch().background({20, 20, 20, 255});
  r.focus_visible = patch().outline(Outline{2.f, {120, 170, 255, 255}, 2.f});
  r.pressed = patch().background({30, 30, 30, 255});
  r.checked = patch().background({40, 40, 40, 255});
  r.active = patch().background({50, 50, 50, 255});
  r.disabled = patch().background({60, 60, 60, 255});
  return r;
}

static bool test_empty_is_base() {
  RoleStyle r = make_role();
  VisualStyle vs = resolve(r, {}, {});
  CHECK(memcmp(&vs, &r.base, sizeof(VisualStyle)) == 0);
  return true;
}

static bool test_variant_before_state() {
  RoleStyle r = make_role();
  StylePatch variant = patch().background({1, 2, 3, 255});
  // variant applies, then hover overrides it (state beats variant):
  VisualStyle vs = resolve(r, variant, InteractionState{.hovered = true});
  CHECK(vs.background == (Color{20, 20, 20, 255}));
  // with no active state, the variant survives:
  VisualStyle vs2 = resolve(r, variant, {});
  CHECK(vs2.background == (Color{1, 2, 3, 255}));
  return true;
}

static bool test_precedence_pressed_beats_hover() {
  RoleStyle r = make_role();
  VisualStyle vs = resolve(r, {}, InteractionState{.hovered = true, .pressed = true});
  CHECK(vs.background == (Color{30, 30, 30, 255})); // pressed > hover
  return true;
}

static bool test_active_between_checked_and_disabled() {
  RoleStyle r = make_role();
  VisualStyle a = resolve(r, {}, InteractionState{.checked = true, .active = true});
  CHECK(a.background == (Color{50, 50, 50, 255})); // active > checked
  VisualStyle d = resolve(r, {}, InteractionState{.active = true, .disabled = true});
  CHECK(d.background == (Color{60, 60, 60, 255})); // disabled > active
  return true;
}

static bool test_disabled_wins_last() {
  RoleStyle r = make_role();
  VisualStyle vs = resolve(
      r, {}, InteractionState{.hovered = true, .pressed = true, .disabled = true});
  CHECK(vs.background == (Color{60, 60, 60, 255}));
  return true;
}

static bool test_set_flag_survival_no_sentinel() {
  RoleStyle r{};
  r.base.background = {99, 99, 99, 255};
  // opt(transparent) APPLIES (set-flag, not value):
  r.hover = patch().background({0, 0, 0, 0});
  VisualStyle vs = resolve(r, {}, InteractionState{.hovered = true});
  CHECK(vs.background == (Color{0, 0, 0, 0}));
  // an Opt with a non-zero value but set==false does NOT apply:
  RoleStyle r2{};
  r2.base.background = {99, 99, 99, 255};
  r2.hover.background = Opt<Color>{false, {255, 255, 255, 255}};
  VisualStyle vs2 = resolve(r2, {}, InteractionState{.hovered = true});
  CHECK(vs2.background == (Color{99, 99, 99, 255})); // base survives
  // corner_radius: opt(0.f) applies; {false, 8.f} does not.
  RoleStyle r3{};
  r3.base.corner_radius = 4.f;
  r3.hover.corner_radius = opt(0.f);
  CHECK(resolve(r3, {}, InteractionState{.hovered = true}).corner_radius == 0.f);
  r3.hover.corner_radius = Opt<float>{false, 8.f};
  CHECK(resolve(r3, {}, InteractionState{.hovered = true}).corner_radius == 4.f);
  return true;
}

static bool test_focus_visible_gate() {
  RoleStyle r = make_role();
  // focused but NOT focus_visible => the focus_visible patch must NOT apply:
  VisualStyle vs = resolve(r, {}, InteractionState{.focused = true, .focus_visible = false});
  CHECK(vs.outline.width == 0.f);
  VisualStyle vs2 = resolve(r, {}, InteractionState{.focused = true, .focus_visible = true});
  CHECK(vs2.outline.width > 0.f);
  return true;
}

int main() {
  bool ok = test_empty_is_base() && test_variant_before_state() &&
            test_precedence_pressed_beats_hover() &&
            test_active_between_checked_and_disabled() &&
            test_disabled_wins_last() && test_set_flag_survival_no_sentinel() &&
            test_focus_visible_gate();
  if (!ok) {
    fprintf(stderr, "ui_style_resolve_tests: FAIL\n");
    return 1;
  }
  printf("ui_style_resolve_tests: OK\n");
  return 0;
}
