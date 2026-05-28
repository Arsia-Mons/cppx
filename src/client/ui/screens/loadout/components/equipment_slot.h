#pragma once

#include "../../../../../ui/retained/element.h"

namespace shooter {

// Renders an equipment-slot button. Updates the loadout's UI selection on
// focus/confirm via `LoadoutContext` — does NOT mutate game state. (The
// player's equipped weapon changes through the confirm dialog's Equip flow.)
::ui::retained::UiElement EquipmentSlot(::ui::retained::UiElementFrame &frame,
                                        const char *id, const char *label,
                                        int weapon_index);

} // namespace shooter
