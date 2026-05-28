#pragma once

#include "../../../ui/runtime/element.h"

#include <functional>

namespace client::ui {

// Struct value provided through AppShellContext.
struct AppShellContextValue {
    std::function<void()> request_quit = {};
};

::ui::UiElement AppShellProvider(const AppShellContextValue &value,
                                 ::ui::UiChildren children);

// Hook: returns the request_quit callback installed by the app. Missing
// providers are reported to the React runtime and return an empty callback.
std::function<void()> use_request_quit();

} // namespace client::ui
