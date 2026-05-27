#include "shooter_game.h"

#include "economy.h"

namespace shooter {

int ShooterGame::ammo() const {
    return inventory_.weapon(inventory_.selected()).spec.ammo;
}

bool ShooterGame::can_buy_weapon(int index) const {
    return economy::can_buy_weapon(player_, inventory_, index);
}

bool ShooterGame::buy_weapon(int index) {
    return economy::try_buy_weapon(player_, inventory_, index);
}

void ShooterGame::reset() {
    *this = ShooterGame();
}

} // namespace shooter
