#pragma once

template <typename Children>
void ComposeLoadout(Children children) {
#line 5 "tests/fixtures/cppx/components.hx"
  Loadout::Frame({ .key = "loadout" }, [&] {
#line 6 "tests/fixtures/cppx/components.hx"
    Loadout::Toolbar({ .activeTab = state.active_tab });
#line 7 "tests/fixtures/cppx/components.hx"
    Loadout::Content({}, [&] {
#line 7 "tests/fixtures/cppx/components.hx"
    children();
#line 7 "tests/fixtures/cppx/components.hx"
    });
#line 8 "tests/fixtures/cppx/components.hx"
  });
}
