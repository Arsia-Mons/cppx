#include "app_shell_provider.h"

#include "../../../react.h"

namespace client::ui {

static ReactContext AppShellContext = {};

::ui::UiElement AppShellProvider(const AppShellContextValue &value,
                                 ::ui::UiChildren children) {
    const AppShellContextValue *stored = ::ui::copy_value(value);
    if (!stored)
        return ::ui::empty();
    return ::ui::provider("AppShellProvider", &AppShellContext,
                          const_cast<AppShellContextValue *>(stored),
                          children);
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
