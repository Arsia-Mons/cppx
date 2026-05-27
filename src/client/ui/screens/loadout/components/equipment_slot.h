#pragma once

#include <clay.h>

namespace shooter {

void EquipmentSlot(Clay_ElementId id,
                   const char    *label,
                   int            weapon_index,
                   int           *selected_index);

} // namespace shooter
