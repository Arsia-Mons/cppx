#include "shooter_game.h"

namespace shooter {

ShooterGame::ShooterGame() {
    weapons_[0] = {
        .spec = { .name = "Vandal", .role = "Rifle", .cost = 0, .damage = 38, .ammo = 24 },
        .owned = true,
        .equipped = true,
    };
    weapons_[1] = {
        .spec = { .name = "Bulldog", .role = "Burst rifle", .cost = 300, .damage = 32, .ammo = 30 },
    };
    weapons_[2] = {
        .spec = { .name = "Operator", .role = "Sniper", .cost = 700, .damage = 120, .ammo = 5 },
    };
    weapons_[3] = {
        .spec = { .name = "Aegis", .role = "Armor kit", .cost = 250, .damage = 0, .ammo = 1 },
    };
}

int ShooterGame::ammo() const {
    return weapons_[selected_weapon_].spec.ammo;
}

bool ShooterGame::can_buy_weapon(int index) const {
    if (index < 0 || index >= weapon_count()) return false;
    const WeaponState &item = weapons_[index];
    return !item.owned && credits_ >= item.spec.cost;
}

bool ShooterGame::can_equip_weapon(int index) const {
    if (index < 0 || index >= weapon_count()) return false;
    return weapons_[index].owned;
}

void ShooterGame::select_weapon(int index) {
    if (index < 0 || index >= weapon_count()) return;
    selected_weapon_ = index;
}

bool ShooterGame::buy_weapon(int index) {
    if (!can_buy_weapon(index)) return false;
    credits_ -= weapons_[index].spec.cost;
    weapons_[index].owned = true;
    return true;
}

bool ShooterGame::equip_weapon(int index) {
    if (!can_equip_weapon(index)) return false;
    for (WeaponState &item : weapons_) {
        item.equipped = false;
    }
    weapons_[index].equipped = true;
    selected_weapon_ = index;
    return true;
}

} // namespace shooter
