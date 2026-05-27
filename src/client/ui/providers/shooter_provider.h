#pragma once

#include <functional>

#include "../../../game/shooter_game.h"

namespace shooter {

struct ShooterContext {
    ShooterGame *game = nullptr;
    std::function<void()> request_quit = {};
};

ShooterGame          *use_shooter_game();
std::function<void()> use_request_quit();

void ShooterProvider(ShooterGame *game,
                     const std::function<void()> &request_quit,
                     const std::function<void()> &children);

} // namespace shooter
