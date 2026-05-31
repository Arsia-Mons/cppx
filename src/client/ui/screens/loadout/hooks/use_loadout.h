#pragma once

#include <functional>
#include <stdint.h>

namespace shooter {

struct LoadoutValue {
  enum class Action {
    None,
    Buy,
    Equip,
  };

  // `generation` is bumped each time a new pending action is set so the dialog
  // can key its fiber identity on it.
  struct PendingAction {
    Action action = Action::None;
    int weapon_index = 0;
    uint32_t generation = 0;
  };

  bool compare_enabled = false;
  int selected_weapon_tile = 0;
  PendingAction pending = {};

  std::function<void(bool)> set_compare_enabled = {};
  std::function<void(int)> set_selected_weapon_tile = {};
  std::function<void(PendingAction)> set_pending_action = {};
  std::function<void()> clear_pending_action = {};
};

LoadoutValue use_loadout();

} // namespace shooter
