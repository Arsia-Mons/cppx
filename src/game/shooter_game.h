#pragma once

#include "inventory.h"
#include "player_state.h"
#include "weapon_state.h"

namespace shooter {

class ShooterGame {
public:
    int health()          const { return player_.health; }
    int armor()           const { return player_.armor; }
    int credits()         const { return player_.credits; }
    int selected_weapon() const { return inventory_.selected(); }
    int ammo()            const;

    const WeaponState &weapon(int index) const { return inventory_.weapon(index); }
    int  weapon_count()      const { return inventory_.weapon_count(); }
    bool can_buy_weapon(int index)   const;
    bool can_equip_weapon(int index) const { return inventory_.can_equip(index); }

    void select_weapon(int index) { inventory_.select(index); }
    bool buy_weapon(int index);
    bool equip_weapon(int index)  { return inventory_.equip(index); }
    void reset();

    PlayerState &player()    { return player_; }
    Inventory   &inventory() { return inventory_; }

private:
    PlayerState player_    = {};
    Inventory   inventory_ = {};
};

} // namespace shooter
