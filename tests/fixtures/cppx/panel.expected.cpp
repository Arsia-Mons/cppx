#include "panel_components.h"

UiElement BuildPauseMenu(UiElementFrame &frame, Actions &actions) {
#line 4 "tests/fixtures/cppx/panel.cppx"
  return Panel(frame, { .key = "pause", .width = Length::points(320), .modal = true, .children = frame.children({
#line 5 "tests/fixtures/cppx/panel.cppx"
    Panel::Header(frame, { .children = frame.children({
#line 6 "tests/fixtures/cppx/panel.cppx"
      Text(frame, { .value = "Paused" }),
#line 7 "tests/fixtures/cppx/panel.cppx"
    }) }),
#line 8 "tests/fixtures/cppx/panel.cppx"
    Panel::Body(frame, { .children = frame.children({
#line 9 "tests/fixtures/cppx/panel.cppx"
      Button(frame, { .id = "ResumeButton", .on_confirm = actions.resume, .children = frame.children({
#line 9 "tests/fixtures/cppx/panel.cppx"
      frame.text("Resume"),
#line 9 "tests/fixtures/cppx/panel.cppx"
      }) }),
#line 10 "tests/fixtures/cppx/panel.cppx"
      Button(frame, { .id = "OptionsButton", .on_confirm = actions.options }),
#line 11 "tests/fixtures/cppx/panel.cppx"
    }) }),
#line 12 "tests/fixtures/cppx/panel.cppx"
  }) });
}
