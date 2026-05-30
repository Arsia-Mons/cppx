#pragma once

// LoadoutTitle: the loadout screen heading. Adapts ui::components::Text directly
// (loadout-local: reserved height 30 differs from the shared ScreenTitle's 32),
// painting kTextTitle at kFontScreenTitle.

#include "ui/components/common.h"

namespace shooter {

struct LoadoutTitleProps {
  const char *key = nullptr;
  const char *value = nullptr;
};

::ui::UiElement LoadoutTitle(const LoadoutTitleProps &props);

} // namespace shooter
