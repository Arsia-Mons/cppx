#include "shooter_hud.h"

#include "../providers/shooter_provider.h"
#include "../../../game/shooter_game.h"

namespace shooter {

ShooterHudRead use_shooter_hud() {
    ShooterGame *game = use_shooter_game();
    if (!game) return {};
    return {
        .health  = game->health(),
        .armor   = game->armor(),
        .ammo    = game->ammo(),
        .credits = game->credits(),
        .weapon  = game->weapon(game->selected_weapon()).spec.name,
    };
}

} // namespace shooter
