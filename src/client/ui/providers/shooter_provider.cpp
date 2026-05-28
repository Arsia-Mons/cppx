#include "shooter_provider.h"

#include "../../../react.h"

namespace shooter {

static ReactContext ShooterContext = {};

::ui::UiElement ShooterProvider(const ShooterContextValue &value,
                                ::ui::UiChildren children) {
    const ShooterContextValue *stored = ::ui::copy_value(value);
    if (!stored)
        return ::ui::empty();
    return ::ui::provider("ShooterProvider", &ShooterContext,
                          const_cast<ShooterContextValue *>(stored), children);
}

ShooterGame *use_shooter_game() {
    ShooterContextValue *value =
        static_cast<ShooterContextValue *>(use_context(&ShooterContext));
    if (!value || !value->game) {
        react_report_error("shooter: missing ShooterProvider\n");
        return nullptr;
    }
    return value->game;
}

} // namespace shooter
