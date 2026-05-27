#include "app_shell.h"

#include "../../../react.h"

namespace client::ui {

static ReactContext AppShellContext = {};

void app_shell_provider_push(const AppShellContextValue *value) {
    react_provider_push(&AppShellContext,
                        const_cast<AppShellContextValue *>(value));
}

void app_shell_provider_pop() {
    react_provider_pop(&AppShellContext);
}

std::function<void()> use_request_quit() {
    AppShellContextValue *value =
        static_cast<AppShellContextValue *>(use_context(&AppShellContext));
    return value ? value->request_quit : std::function<void()>{};
}

} // namespace client::ui
