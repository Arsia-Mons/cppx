#include "panel_components.h"

UiElement BuildMultiline(UiElementFrame &frame, Actions &actions) {
#line 4 "tests/fixtures/cppx/multiline.cppx"
  return Panel(frame, { .key = "pause", .style = Style{ .width = Length::points(320), .height = Length::points(180), }, .modal = true, .children = frame.children({
#line 11 "tests/fixtures/cppx/multiline.cppx"
    Button(frame, { .id = "ResumeButton", .on_activate = actions.resume, .children = frame.children({
#line 14 "tests/fixtures/cppx/multiline.cppx"
      frame.text("Resume"),
#line 15 "tests/fixtures/cppx/multiline.cppx"
    }) }),
#line 16 "tests/fixtures/cppx/multiline.cppx"
  }) });
}
