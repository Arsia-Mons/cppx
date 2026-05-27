#include "shooter_provider.h"

#include "../../../react.h"

namespace shooter {

static ReactContext ShooterContext = {};

void shooter_provider_push(const ShooterContextValue *value) {
    react_provider_push(&ShooterContext,
                        const_cast<ShooterContextValue *>(value));
}

void shooter_provider_pop() {
    react_provider_pop(&ShooterContext);
}

ShooterGame *use_shooter_game() {
    ShooterContextValue *value =
        static_cast<ShooterContextValue *>(use_context(&ShooterContext));
    return value ? value->game : nullptr;
}

} // namespace shooter
