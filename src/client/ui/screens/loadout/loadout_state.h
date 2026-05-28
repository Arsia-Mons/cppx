#pragma once

#include <functional>
#include <stdint.h>

#include "../../../../ui/runtime/element.h"

namespace shooter {

// Pending confirmation-dialog action. `generation` is bumped each time a new
// pending action is set so the dialog can key its fiber identity on it.
constexpr int LOADOUT_ACTION_NONE = 0;
constexpr int LOADOUT_ACTION_BUY = 1;
constexpr int LOADOUT_ACTION_EQUIP = 2;

struct LoadoutPendingAction {
  int action = LOADOUT_ACTION_NONE;
  int weapon_index = 0;
  uint32_t generation = 0;
};

// Struct value provided through LoadoutContext. The owning screen body holds
// the actual state in `use_state<...>` slots and exposes them via pointers +
// named setters that schedule deferred UI mutations.
struct LoadoutContextValue {
  bool *compare_enabled = nullptr;
  int *selected_weapon_tile = nullptr;
  LoadoutPendingAction *pending = nullptr;
  std::function<void(bool)> set_compare_enabled = {};
  std::function<void(int)> set_selected_weapon_tile = {};
  std::function<void(LoadoutPendingAction)> set_pending_action = {};
  std::function<void()> clear_pending_action = {};
};

LoadoutContextValue use_loadout_context_value(bool *compare_enabled,
                                              int *selected_weapon_tile,
                                              LoadoutPendingAction *pending);
::ui::UiElement LoadoutProvider(const LoadoutContextValue &value,
                                ::ui::UiChildren children);

// Hooks: read state.
bool use_compare_enabled();
int use_selected_weapon_tile();
LoadoutPendingAction use_pending_loadout_action();

// Hooks: write state. Setters schedule deferred UI mutations so they're safe to
// invoke from inside the UI declaration pass.
std::function<void(bool)> use_set_compare_enabled();
std::function<void(int)> use_set_selected_weapon_tile();
std::function<void(LoadoutPendingAction)> use_set_pending_loadout_action();
std::function<void()> use_clear_pending_loadout_action();

} // namespace shooter
