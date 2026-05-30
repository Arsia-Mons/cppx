#pragma once

#include <functional>

namespace shooter {

// Navigation action: push a fresh LoadoutScreen onto the screen stack. Consumed
// by the pause screen (via loadout_screen.h re-include). Moved out of
// loadout_screen.cpp so the screen entry point stays free of nav-hook glue.
std::function<void()> use_push_loadout_screen();

} // namespace shooter
