#pragma once

template <typename Children>
UiElement ComposeLoadout(Children children) {
#line 5 "tests/fixtures/cppx/components.hx"
  return Loadout::Frame({ .key = "loadout", .children = children({
#line 6 "tests/fixtures/cppx/components.hx"
    Loadout::Toolbar({ .active_tab = state.active_tab }),
#line 7 "tests/fixtures/cppx/components.hx"
    Loadout::Content({ .children = children({
#line 7 "tests/fixtures/cppx/components.hx"
    children(),
#line 7 "tests/fixtures/cppx/components.hx"
    }) }),
#line 8 "tests/fixtures/cppx/components.hx"
  }) });
}
