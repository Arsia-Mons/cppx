#pragma once

#include "../../../game/shooter_game.h"

namespace shooter {

// Struct value provided through ShooterContext.
struct ShooterContextValue {
    ShooterGame *game = nullptr;
};

// Push/pop the shooter game context. The app wires this around the screen
// stack so descendant components can call use_shooter_game() directly.
void shooter_provider_push(const ShooterContextValue *value);
void shooter_provider_pop();

// Hook: returns the game pointer installed by the active provider, or nullptr
// if no provider is in scope.
ShooterGame *use_shooter_game();

} // namespace shooter
