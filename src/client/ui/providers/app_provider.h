#pragma once

#include "ui/runtime/element.h"

#include <functional>

namespace client::ui {

struct AppProviderValue {
  std::function<void()> quit = {};
};

::ui::UiElement AppProvider(const AppProviderValue &value,
                            ::ui::UiChildren children);

} // namespace client::ui
