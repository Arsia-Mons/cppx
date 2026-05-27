#include "inventory.h"

namespace shooter {

Inventory::Inventory() {
    weapons_[0] = {
        .spec     = { .name = "Vandal",   .role = "Rifle",       .cost = 0,   .damage = 38,  .ammo = 24 },
        .owned    = true,
        .equipped = true,
    };
    weapons_[1] = {
        .spec = { .name = "Bulldog",  .role = "Burst rifle", .cost = 300, .damage = 32,  .ammo = 30 },
    };
    weapons_[2] = {
        .spec = { .name = "Operator", .role = "Sniper",      .cost = 700, .damage = 120, .ammo = 5  },
    };
    weapons_[3] = {
        .spec = { .name = "Aegis",    .role = "Armor kit",   .cost = 250, .damage = 0,   .ammo = 1  },
    };
}

bool Inventory::can_equip(int index) const {
    if (index < 0 || index >= weapon_count()) return false;
    return weapons_[index].owned;
}

void Inventory::select(int index) {
    if (index < 0 || index >= weapon_count()) return;
    selected_ = index;
}

bool Inventory::equip(int index) {
    if (!can_equip(index)) return false;
    for (WeaponState &item : weapons_) item.equipped = false;
    weapons_[index].equipped = true;
    selected_ = index;
    return true;
}

void Inventory::mark_owned(int index) {
    if (index < 0 || index >= weapon_count()) return;
    weapons_[index].owned = true;
}

void Inventory::reset() {
    *this = Inventory();
}

} // namespace shooter
