#pragma once

#include "../../../game/shooter_game.h"
#include "../../../ui/runtime/element.h"

namespace shooter {

// Struct value provided through ShooterContext.
struct ShooterContextValue {
    ShooterGame *game = nullptr;
};

::ui::UiElement ShooterProvider(const ShooterContextValue &value,
                                ::ui::UiChildren children);

// Hook: returns the game pointer installed by the active provider. Missing
// providers are reported to the React runtime and return nullptr.
ShooterGame *use_shooter_game();

} // namespace shooter
