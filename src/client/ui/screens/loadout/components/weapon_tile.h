#pragma once

#include <clay.h>

namespace shooter {

constexpr int LOADOUT_TAB_WEAPONS = 0;
constexpr int LOADOUT_TAB_GEAR    = 1;

Clay_ElementId weapon_tile_id(int index);
bool           weapon_in_tab(int index, int tab);
int            first_weapon_for_tab(int tab);

void WeaponTile(int index, int *selected_index);

} // namespace shooter
