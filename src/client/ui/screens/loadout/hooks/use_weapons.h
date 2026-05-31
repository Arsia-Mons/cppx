#pragma once

#include <functional>

namespace shooter {

class ShooterGame;

class WeaponsValue {
public:
  struct Weapon {
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

  int count() const;
  Weapon get_weapon(int index) const;

  void select(int index) const;
  void buy(int index) const;
  void equip(int index) const;

private:
  friend WeaponsValue use_weapons();

  WeaponsValue(ShooterGame *game, std::function<void(int)> select,
               std::function<void(int)> buy, std::function<void(int)> equip);

  ShooterGame *game_ = nullptr;
  std::function<void(int)> select_ = {};
  std::function<void(int)> buy_ = {};
  std::function<void(int)> equip_ = {};
};

WeaponsValue use_weapons();

} // namespace shooter
