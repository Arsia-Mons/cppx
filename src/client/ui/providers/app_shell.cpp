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
    if (!value) {
        react_report_error("client/ui: missing AppShellProvider\n");
        return {};
    }
    return value->request_quit;
}

} // namespace client::ui
