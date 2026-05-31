#include "use_weapons.h"

#include "client/ui/app_shell/deferred_ui_mutation.h"
#include "client/ui/callback_deps.h"
#include "client/ui/hooks/use_server.h"
#include "game/shooter_game.h"
#include "react.h"

#include <stdint.h>

namespace shooter {

namespace {

template <typename Mutation>
std::function<void(int)> use_weapon_action(ShooterGame *game,
                                           uint64_t action_id,
                                           Mutation mutation) {
  client::ui::internal::DeferredUiMutationSink mutations =
      client::ui::internal::use_deferred_ui_mutations();
  return use_callback<void(int)>(
      [game, mutation, mutations](int index) {
        if (game && mutations && index >= 0 && index < game->weapon_count()) {
          mutations.submit([game, index, mutation] { (game->*mutation)(index); });
        }
      },
      client::ui::callback_deps(
          client::ui::callback_deps_ptr(game),
          client::ui::callback_deps_ptr(mutations.owner()),
          action_id));
}

} // namespace

WeaponsValue::WeaponsValue(ShooterGame *game, std::function<void(int)> select,
                           std::function<void(int)> buy,
                           std::function<void(int)> equip)
    : game_(game), select_(select), buy_(buy), equip_(equip) {}

int WeaponsValue::count() const { return game_ ? game_->weapon_count() : 0; }

WeaponsValue::Weapon WeaponsValue::get_weapon(int index) const {
  if (!game_ || index < 0 || index >= game_->weapon_count())
    return {};

  const WeaponState &state = game_->weapon(index);
  bool can_buy = game_->can_buy_weapon(index);
  bool can_equip = game_->can_equip_weapon(index);
  return {
      .valid = true,
      .name = state.spec.name,
      .role = state.spec.role,
      .cost = state.spec.cost,
      .damage = state.spec.damage,
      .ammo = state.spec.ammo,
      .owned = state.owned,
      .equipped = state.equipped,
      .can_buy = can_buy,
      .can_equip = can_equip,
      .disabled = !state.owned && !can_buy,
  };
}

void WeaponsValue::select(int index) const {
  if (select_)
    select_(index);
}

void WeaponsValue::buy(int index) const {
  if (buy_)
    buy_(index);
}

void WeaponsValue::equip(int index) const {
  if (equip_)
    equip_(index);
}

WeaponsValue use_weapons() {
  ShooterGame *game = use_server().game;
  return WeaponsValue(
      game, use_weapon_action(game, 1, &ShooterGame::select_weapon),
      use_weapon_action(game, 2, &ShooterGame::buy_weapon),
      use_weapon_action(game, 3, &ShooterGame::equip_weapon));
}

} // namespace shooter
