#pragma once

namespace shooter {

struct PlayerState;
class  Inventory;

namespace economy {

bool can_buy_weapon(const PlayerState &player, const Inventory &inventory, int index);
bool try_buy_weapon(PlayerState &player, Inventory &inventory, int index);

} // namespace economy
} // namespace shooter
