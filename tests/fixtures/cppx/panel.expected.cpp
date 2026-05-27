#include "panel_components.h"

void BuildPauseMenu(Actions &actions) {
#line 4 "tests/fixtures/cppx/panel.cppx"
  Panel({ .key = "pause", .width = Length::points(320), .modal = true }, [&] {
#line 5 "tests/fixtures/cppx/panel.cppx"
    Panel::Header({}, [&] {
#line 6 "tests/fixtures/cppx/panel.cppx"
      Text({ .value = "Paused" });
#line 7 "tests/fixtures/cppx/panel.cppx"
    });
#line 8 "tests/fixtures/cppx/panel.cppx"
    Panel::Body({}, [&] {
#line 9 "tests/fixtures/cppx/panel.cppx"
      Button({ .id = "ResumeButton", .onConfirm = actions.resume }, [&] {
#line 9 "tests/fixtures/cppx/panel.cppx"
      cppx_text("Resume");
#line 9 "tests/fixtures/cppx/panel.cppx"
      });
#line 10 "tests/fixtures/cppx/panel.cppx"
      Button({ .id = "OptionsButton", .onConfirm = actions.options });
#line 11 "tests/fixtures/cppx/panel.cppx"
    });
#line 12 "tests/fixtures/cppx/panel.cppx"
  });
}
