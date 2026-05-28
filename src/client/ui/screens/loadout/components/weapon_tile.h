#pragma once

#include "../../../../../ui/runtime/element.h"

namespace shooter {

constexpr int LOADOUT_TAB_WEAPONS = 0;
constexpr int LOADOUT_TAB_GEAR = 1;
constexpr const char *WEAPON_TILE_CONTROL_ID = "WeaponTile";

bool weapon_in_tab(int index, int tab);
int first_weapon_for_tab(int tab);
const char *weapon_tile_key(int index);

struct WeaponTileProps {
  const char *key = nullptr;
  int index = 0;
};

// Renders one weapon tile. Reads/writes the loadout's UI selection through
// `LoadoutContext` — does NOT mutate game state. The Equip flow goes through
// the confirm dialog, not through focus.
::ui::UiElement WeaponTile(const WeaponTileProps &props);

} // namespace shooter
