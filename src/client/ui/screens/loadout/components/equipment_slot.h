#pragma once

#include "../../../../../ui/runtime/element.h"

namespace shooter {

struct EquipmentSlotProps {
  const char *key = nullptr;
  const char *id = nullptr;
  const char *label = nullptr;
  int weapon_index = 0;
};

// Renders an equipment-slot button. Updates the loadout's UI selection on
// focus/confirm via `LoadoutContext`; the player's equipped weapon changes
// through the confirm dialog's Equip flow.
::ui::UiElement EquipmentSlot(const EquipmentSlotProps &props);

} // namespace shooter
