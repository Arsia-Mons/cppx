#pragma once

namespace shooter {

struct HudValue {
  int health = 0;
  int armor = 0;
  int ammo = 0;
  int credits = 0;
  const char *weapon = "";
};

HudValue use_hud();

} // namespace shooter
