#include "panel_components.h"

UiElement BuildPauseMenu(Actions &actions) {
#line 4 "tests/fixtures/cppx/panel.cppx"
  return Panel({ .key = "pause", .style = Style{.width = Length::points(320)}, .modal = true, .children = children({
#line 5 "tests/fixtures/cppx/panel.cppx"
    Panel::Header({ .children = children({
#line 6 "tests/fixtures/cppx/panel.cppx"
      Text({ .value = "Paused" }),
#line 7 "tests/fixtures/cppx/panel.cppx"
    }) }),
#line 8 "tests/fixtures/cppx/panel.cppx"
    Panel::Body({ .children = children({
#line 9 "tests/fixtures/cppx/panel.cppx"
      Button({ .id = "ResumeButton", .on_activate = actions.resume, .children = children({
#line 9 "tests/fixtures/cppx/panel.cppx"
      text("Resume"),
#line 9 "tests/fixtures/cppx/panel.cppx"
      }) }),
#line 10 "tests/fixtures/cppx/panel.cppx"
      Button({ .id = "OptionsButton", .on_activate = actions.options }),
#line 11 "tests/fixtures/cppx/panel.cppx"
    }) }),
#line 12 "tests/fixtures/cppx/panel.cppx"
  }) });
}
