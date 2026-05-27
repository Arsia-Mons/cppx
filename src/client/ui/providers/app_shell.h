#pragma once

#include <functional>

namespace client::ui {

// Struct value provided through AppShellContext.
struct AppShellContextValue {
    std::function<void()> request_quit = {};
};

// Push/pop the app-shell context. App-level code wraps the screen stack with
// these so consumer screens can read `use_request_quit()` without each screen
// re-installing its own provider.
void app_shell_provider_push(const AppShellContextValue *value);
void app_shell_provider_pop();

// Hook: returns the request_quit callback installed by the app. Missing
// providers are reported to the React runtime and return an empty callback.
std::function<void()> use_request_quit();

} // namespace client::ui
