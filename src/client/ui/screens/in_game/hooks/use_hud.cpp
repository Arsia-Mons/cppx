#include "use_hud.h"

#include "client/ui/hooks/use_server.h"
#include "game/shooter_game.h"

namespace shooter {

HudValue use_hud() {
  ShooterGame *game = use_server().game;
  if (!game)
    return {};
  return {
      .health = game->health(),
      .armor = game->armor(),
      .ammo = game->ammo(),
      .credits = game->credits(),
      .weapon = game->weapon(game->selected_weapon()).spec.name,
  };
}

} // namespace shooter
