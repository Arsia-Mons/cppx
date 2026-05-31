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

// NEW INVARIANT: a per-instance StyleStatePatch owns its own per-state slots.
// resolve() layers the override's state patch OVER the role's state patch for
// each active interaction flag, so a variant can own its own :hover/:pressed/
// :focus_visible/:disabled. Override state wins OVER role state, per field.
static bool test_override_state_beats_role_state() {
  RoleStyle r = make_role();
  // role.hover sets background to A = {20,20,20,255} (from make_role()).
  const Color A{20, 20, 20, 255};
  const Color B{200, 100, 50, 255};
  StyleStatePatch ov{};
  ov.hover = patch().background(B); // override owns its OWN :hover background.
  // hovered: both role.hover and ov.hover fire; override layers last => B wins.
  VisualStyle vs = resolve(r, ov, InteractionState{.hovered = true});
  CHECK(vs.background == B);
  // sanity: without the override, the role's hover (A) is what resolves.
  VisualStyle role_only = resolve(r, {}, InteractionState{.hovered = true});
  CHECK(role_only.background == A);
  CHECK(A != B);
  return true;
}

// The override's state slot only fires for its OWN active flag: an override
// :pressed must NOT leak into a hover-only resolve, and the role's hover still
// resolves untouched. Then assert it DOES win once pressed is active.
static bool test_override_state_gated_by_flag() {
  RoleStyle r = make_role();
  StyleStatePatch ov{};
  ov.pressed = patch().background({7, 8, 9, 255});
  // hover only: override.pressed must stay dormant; role.hover ({20,20,20}) wins.
  VisualStyle hov = resolve(r, ov, InteractionState{.hovered = true});
  CHECK(hov.background == (Color{20, 20, 20, 255}));
  // pressed active: override.pressed layers over role.pressed ({30,30,30}) => wins.
  VisualStyle prs = resolve(r, ov, InteractionState{.hovered = true, .pressed = true});
  CHECK(prs.background == (Color{7, 8, 9, 255}));
  return true;
}

// An EMPTY StyleStatePatch (every slot default — including base) is a no-op
// overlay: resolve(role, {}, st) must equal the pure base->role-state cascade,
// byte/field-exact, for a representative spread of interaction states. We prove
// equality against an independently computed reference (base then role patches),
// not against resolve() itself, so the cascade itself is pinned.
static VisualStyle role_only_reference(const RoleStyle &r,
                                       const InteractionState &st) {
  VisualStyle vs = r.base;
  if (st.hovered)
    apply(vs, r.hover);
  if (st.focus_visible)
    apply(vs, r.focus_visible);
  if (st.pressed)
    apply(vs, r.pressed);
  if (st.checked)
    apply(vs, r.checked);
  if (st.active)
    apply(vs, r.active);
  if (st.disabled)
    apply(vs, r.disabled);
  return vs;
}

static bool test_empty_state_patch_is_role_only_noop() {
  RoleStyle r = make_role();
  const InteractionState states[] = {
      {},
      InteractionState{.hovered = true},
      InteractionState{.focused = true, .focus_visible = true},
      InteractionState{.hovered = true, .pressed = true},
      InteractionState{.checked = true, .active = true},
      InteractionState{.hovered = true, .pressed = true, .disabled = true},
  };
  for (const InteractionState &st : states) {
    VisualStyle empty_ov = resolve(r, StyleStatePatch{}, st);
    VisualStyle ref = role_only_reference(r, st);
    CHECK(memcmp(&empty_ov, &ref, sizeof(VisualStyle)) == 0);
    // A bare StylePatch implicitly converts to the base-only form, which is
    // likewise a no-op overlay when the StylePatch is empty.
    VisualStyle bare_ov = resolve(r, StylePatch{}, st);
    CHECK(memcmp(&bare_ov, &ref, sizeof(VisualStyle)) == 0);
  }
  return true;
}

int main() {
  bool ok = test_empty_is_base() && test_variant_before_state() &&
            test_precedence_pressed_beats_hover() &&
            test_active_between_checked_and_disabled() &&
            test_disabled_wins_last() && test_set_flag_survival_no_sentinel() &&
            test_focus_visible_gate() &&
            test_override_state_beats_role_state() &&
            test_override_state_gated_by_flag() &&
            test_empty_state_patch_is_role_only_noop();
  if (!ok) {
    fprintf(stderr, "ui_style_resolve_tests: FAIL\n");
    return 1;
  }
  printf("ui_style_resolve_tests: OK\n");
  return 0;
}
