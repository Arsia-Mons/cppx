#include "economy.h"

#include "inventory.h"
#include "player_state.h"

namespace shooter::economy {

bool can_buy_weapon(const PlayerState &player, const Inventory &inventory, int index) {
    if (index < 0 || index >= inventory.weapon_count()) return false;
    const WeaponState &item = inventory.weapon(index);
    return !item.owned && player.can_afford(item.spec.cost);
}

bool try_buy_weapon(PlayerState &player, Inventory &inventory, int index) {
    if (!can_buy_weapon(player, inventory, index)) return false;
    player.spend(inventory.weapon(index).spec.cost);
    inventory.mark_owned(index);
    return true;
}

} // namespace shooter::economy
