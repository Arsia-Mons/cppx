#include "shooter_provider.h"

#include <stdint.h>

#include "../../../react.h"

namespace shooter {

static ReactContext ShooterContextValue = {};

ShooterGame *use_shooter_game() {
    ShooterContext *context =
        static_cast<ShooterContext *>(use_context(&ShooterContextValue));
    return context ? context->game : nullptr;
}

std::function<void()> use_request_quit() {
    ShooterContext *context =
        static_cast<ShooterContext *>(use_context(&ShooterContextValue));
    return context ? context->request_quit : std::function<void()>{};
}

void ShooterProvider(ShooterGame *game,
                     const std::function<void()> &request_quit,
                     const std::function<void()> &children) {
    ShooterContext context = { .game = game, .request_quit = request_quit };
    REACT_PROVIDER_ENTER_KEY("ShooterProvider", reinterpret_cast<uintptr_t>(game));
    PROVIDE(&ShooterContextValue, &context) {
        children();
    }
    REACT_PROVIDER_EXIT();
}

} // namespace shooter
