#pragma once

namespace shooter {

struct WeaponSpec {
    const char *name   = "";
    const char *role   = "";
    int         cost   = 0;
    int         damage = 0;
    int         ammo   = 0;
};

} // namespace shooter
