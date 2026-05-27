#pragma once

#include <functional>

namespace shooter {

struct ShooterWeaponRead {
    bool valid = false;
    const char *name = "";
    const char *role = "";
    int cost = 0;
    int damage = 0;
    int ammo = 0;
    bool owned = false;
    bool equipped = false;
    bool can_buy = false;
    bool can_equip = false;
    bool disabled = true;
};

int                   use_shooter_weapon_count();
int                   use_selected_weapon_index();
ShooterWeaponRead     use_weapon_read(int index);
std::function<void()> use_select_weapon(int index);
std::function<void()> use_buy_weapon(int index);
std::function<void()> use_equip_weapon(int index);

} // namespace shooter
