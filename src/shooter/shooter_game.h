#pragma once

#include <array>

namespace shooter {

constexpr int SHOOTER_WEAPON_COUNT = 4;

struct WeaponSpec {
    const char *name = "";
    const char *role = "";
    int cost = 0;
    int damage = 0;
    int ammo = 0;
};

struct WeaponState {
    WeaponSpec spec = {};
    bool owned = false;
    bool equipped = false;
};

class ShooterGame {
public:
    ShooterGame();

    int health() const { return health_; }
    int armor() const { return armor_; }
    int ammo() const;
    int credits() const { return credits_; }
    int selected_weapon() const { return selected_weapon_; }
    bool compare_enabled() const { return compare_enabled_; }

    const WeaponState &weapon(int index) const { return weapons_[index]; }
    int weapon_count() const { return SHOOTER_WEAPON_COUNT; }
    bool can_buy_weapon(int index) const;
    bool can_equip_weapon(int index) const;

    void select_weapon(int index);
    bool buy_weapon(int index);
    bool equip_weapon(int index);
    void set_compare_enabled(bool enabled) { compare_enabled_ = enabled; }

private:
    int health_ = 86;
    int armor_ = 42;
    int credits_ = 450;
    int selected_weapon_ = 0;
    bool compare_enabled_ = false;
    std::array<WeaponState, SHOOTER_WEAPON_COUNT> weapons_ = {};
};

} // namespace shooter
