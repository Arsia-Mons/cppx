#pragma once

// LoadoutScreenBody: the loadout content composer. Authored as plain C++ (not
// JSX) because its body returns a fragment of two siblings and branches/builds
// closures — the transpiler only enters JSX mode on a single-root element. It
// owns every loadout hook read + seed-clamp + closure (the 2-deferred-write
// gear-tab contract lives in the gear on_select built here) and assembles the
// semantic sub-component tree. The "LoadoutScreenBody" display name is preserved
// by the screen's ::ui::component registration so fiber identity is unchanged.

#include "ui/components/common.h"

#include <cstdint>

namespace shooter {

struct LoadoutScreenBodyProps {
  uint32_t unused = 0;
};

::ui::UiElement LoadoutScreenBody(const LoadoutScreenBodyProps &props);

} // namespace shooter
