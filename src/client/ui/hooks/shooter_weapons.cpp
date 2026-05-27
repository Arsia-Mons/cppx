#include "shooter_weapons.h"

#include "../../../game/shooter_game.h"
#include "../../../react.h"
#include "../callback_deps.h"
#include "../client_ui.h"
#include "../internal/deferred_ui_mutation.h"
#include "../providers/shooter_provider.h"

namespace shooter {

int use_shooter_weapon_count() {
    ShooterGame *game = use_shooter_game();
    return game ? game->weapon_count() : 0;
}

int use_selected_weapon_index() {
    ShooterGame *game = use_shooter_game();
    return game ? game->selected_weapon() : 0;
}

ShooterWeaponRead use_weapon_read(int index) {
    ShooterGame *game = use_shooter_game();
    if (!game || index < 0 || index >= game->weapon_count())
        return {};
    const WeaponState &weapon = game->weapon(index);
    bool can_buy = game->can_buy_weapon(index);
    bool can_equip = game->can_equip_weapon(index);
    return {
        .valid = true,
        .name = weapon.spec.name,
        .role = weapon.spec.role,
        .cost = weapon.spec.cost,
        .damage = weapon.spec.damage,
        .ammo = weapon.spec.ammo,
        .owned = weapon.owned,
        .equipped = weapon.equipped,
        .can_buy = can_buy,
        .can_equip = can_equip,
        .disabled = !weapon.owned && !can_buy,
    };
}

std::function<void()> use_select_weapon(int index) {
    ShooterGame *game = use_shooter_game();
    client::ui::internal::DeferredUiMutationSink mutations =
        client::ui::internal::use_deferred_ui_mutations();
    return use_callback(
        [game, index, mutations] {
            if (game && mutations) {
                mutations.submit([game, index] { game->select_weapon(index); });
            }
        },
        client::ui::callback_deps(client::ui::callback_deps_ptr(game),
                                  client::ui::callback_deps_ptr(mutations.owner()),
                                  static_cast<uint64_t>(index)));
}

std::function<void()> use_buy_weapon(int index) {
    ShooterGame *game = use_shooter_game();
    client::ui::internal::DeferredUiMutationSink mutations =
        client::ui::internal::use_deferred_ui_mutations();
    return use_callback(
        [game, index, mutations] {
            if (game && mutations) {
                mutations.submit([game, index] { game->buy_weapon(index); });
            }
        },
        client::ui::callback_deps(client::ui::callback_deps_ptr(game),
                                  client::ui::callback_deps_ptr(mutations.owner()),
                                  static_cast<uint64_t>(index)));
}

std::function<void()> use_equip_weapon(int index) {
    ShooterGame *game = use_shooter_game();
    client::ui::internal::DeferredUiMutationSink mutations =
        client::ui::internal::use_deferred_ui_mutations();
    return use_callback(
        [game, index, mutations] {
            if (game && mutations) {
                mutations.submit([game, index] { game->equip_weapon(index); });
            }
        },
        client::ui::callback_deps(client::ui::callback_deps_ptr(game),
                                  client::ui::callback_deps_ptr(mutations.owner()),
                                  static_cast<uint64_t>(index)));
}

} // namespace shooter
