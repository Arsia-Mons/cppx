#pragma once

#include "ui/components/common.h"

namespace shooter {

struct LoadoutProviderProps {
  const char *key = nullptr;
  ::ui::UiChildren children = {};
};

::ui::UiElement LoadoutProvider(const LoadoutProviderProps &props);

} // namespace shooter
