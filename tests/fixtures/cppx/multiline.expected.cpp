#include "panel_components.h"

UiElement BuildMultiline(Actions &actions) {
#line 4 "tests/fixtures/cppx/multiline.cppx"
  return Panel({ .key = "pause", .style = Style{ .width = Length::points(320), .height = Length::points(180), }, .modal = true, .children = children({
#line 11 "tests/fixtures/cppx/multiline.cppx"
    Button({ .id = "ResumeButton", .on_activate = actions.resume, .children = children({
#line 14 "tests/fixtures/cppx/multiline.cppx"
      text("Resume"),
#line 15 "tests/fixtures/cppx/multiline.cppx"
    }) }),
#line 16 "tests/fixtures/cppx/multiline.cppx"
  }) });
}
