#pragma once

#include "game/shooter_game.h"
#include "ui/runtime/element.h"

namespace shooter {

struct ServerProviderValue {
  ShooterGame *game = nullptr;
};

::ui::UiElement ServerProvider(const ServerProviderValue &value,
                               ::ui::UiChildren children);

} // namespace shooter
