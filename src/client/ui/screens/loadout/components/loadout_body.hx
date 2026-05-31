#pragma once

// LoadoutBody: the horizontal row holding the weapon grid and the details
// column. Adapts ui::components::Box (Row / Start / gap 18).

#include "ui/components/common.h"

namespace shooter {

struct LoadoutBodyProps {
  const char *key = nullptr;
  ::ui::UiChildren children = {};
};

::ui::UiElement LoadoutBody(const LoadoutBodyProps &props);

} // namespace shooter
