#pragma once

#include "game/shooter_game.h"

#include <functional>

namespace shooter {

struct ServerActions {
  std::function<void()> reset_match = {};
};

struct ServerValue {
  ShooterGame *game = nullptr;
  ServerActions actions = {};
};

ServerValue use_server();

} // namespace shooter
