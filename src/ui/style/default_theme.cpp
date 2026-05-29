#include "theme.h"

// default_theme(): the v1 default design system. Seeded from the old common.h
// control palette as a STARTING POINT — there is no parity constraint; these
// values are freely re-chooseable and goldens are authored fresh. See design §4.5.
//
// Which patches are populated:
//   base          - always (dense)
//   disabled      - button/input/checkbox (dimmed fill + border)
//   checked       - checkbox / checkbox_mark
//   focus_visible - every focusable role (the ONLY wiring that produces the focus ring)
//   hover/pressed/active - empty in v1 (left for tuning)

namespace ui {

const Theme &default_theme() {
  static const Theme t = [] {
    Theme th{};
    th.focus_ring = {120, 170, 255, 255};
    th.text_default = {224, 228, 236, 255};
    th.text_disabled = {120, 128, 140, 255};
    th.caret = {224, 228, 236, 255};
    th.selection = {44, 92, 128, 180}; // straight alpha; premultiplied at IR emit

    // Focus ring patch: ONLY outline is set; everything else inherits from base.
    const StylePatch focus_ring_patch =
        patch().outline(Outline{2.f, th.focus_ring, 2.f});

    auto seed_control = [&](RoleStyle &r) {
      r.base.background = {24, 28, 36, 255};
      r.base.border.width = {1, 1, 1, 1};
      r.base.border.color = {{78, 88, 104, 255},
                             {78, 88, 104, 255},
                             {78, 88, 104, 255},
                             {78, 88, 104, 255}};
      r.disabled.background = opt(Color{30, 34, 42, 255});
      r.disabled.border = opt(Border{{1, 1, 1, 1},
                                     {{62, 68, 78, 255},
                                      {62, 68, 78, 255},
                                      {62, 68, 78, 255},
                                      {62, 68, 78, 255}}});
      r.focus_visible = focus_ring_patch; // the locked focus ring
    };
    seed_control(th.button);
    seed_control(th.input);
    seed_control(th.checkbox);

    th.checkbox.checked.background = opt(Color{44, 92, 128, 255});
    th.checkbox_mark.base.background = {0, 0, 0, 0};                  // hidden until checked
    th.checkbox_mark.checked.background = opt(Color{224, 228, 236, 255}); // visible mark

    th.box.base = VisualStyle{};
    th.text.base.text = TextVisual{th.text_default, 0, 14};
    th.dialog.base.background = {18, 21, 27, 245};
    return th;
  }();
  return t;
}

} // namespace ui
