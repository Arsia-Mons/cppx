#pragma once

namespace shooter {

constexpr int LOADOUT_ACTION_NONE  = 0;
constexpr int LOADOUT_ACTION_BUY   = 1;
constexpr int LOADOUT_ACTION_EQUIP = 2;

void LoadoutConfirmDialog(int  action,
                          int  weapon_index,
                          int  serial,
                          int *pending_action);

} // namespace shooter
