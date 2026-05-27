#pragma once

namespace shooter {

struct ShooterHudRead {
    int health = 0;
    int armor = 0;
    int ammo = 0;
    int credits = 0;
    const char *weapon = "";
};

ShooterHudRead use_shooter_hud();

} // namespace shooter
