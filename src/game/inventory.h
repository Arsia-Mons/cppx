#pragma once

#include <array>

#include "weapon_state.h"

namespace shooter {

constexpr int SHOOTER_WEAPON_COUNT = 4;

class Inventory {
public:
    Inventory();

    int weapon_count() const { return SHOOTER_WEAPON_COUNT; }
    const WeaponState &weapon(int index) const { return weapons_[index]; }
          WeaponState &weapon_mut(int index)   { return weapons_[index]; }

    int selected() const { return selected_; }

    bool can_equip(int index) const;

    void select(int index);
    bool equip(int index);
    void mark_owned(int index);
    void reset();

private:
    std::array<WeaponState, SHOOTER_WEAPON_COUNT> weapons_ = {};
    int                                           selected_ = 0;
};

} // namespace shooter
