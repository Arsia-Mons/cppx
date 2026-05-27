#pragma once

#include "weapon_defs.h"

namespace shooter {

struct WeaponState {
    WeaponSpec spec     = {};
    bool       owned    = false;
    bool       equipped = false;
};

} // namespace shooter
