#pragma once

template <typename Children>
UiElement ComposeLoadout(UiElementFrame &frame, Children children) {
#line 5 "tests/fixtures/cppx/components.hx"
  return Loadout::Frame(frame, { .key = "loadout", .children = frame.children({
#line 6 "tests/fixtures/cppx/components.hx"
    Loadout::Toolbar(frame, { .active_tab = state.active_tab }),
#line 7 "tests/fixtures/cppx/components.hx"
    Loadout::Content(frame, { .children = frame.children({
#line 7 "tests/fixtures/cppx/components.hx"
    children(),
#line 7 "tests/fixtures/cppx/components.hx"
    }) }),
#line 8 "tests/fixtures/cppx/components.hx"
  }) });
}
