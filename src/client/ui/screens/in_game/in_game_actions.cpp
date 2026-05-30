#include "in_game_actions.h"

#include <memory>

#include "client/ui/callback_deps.h"
#include "client/ui/app_shell/client_ui.h"
#include "client/ui/app_shell/deferred_ui_mutation.h"
#include "client/ui/providers/shooter_provider.h"
#include "client/ui/screens/in_game/in_game_screen.h"
#include "react.h"

namespace shooter {

std::function<void()> use_start_match() {
  ShooterGame *game = use_shooter_game();
  client::ui::ScreenNavigator nav = client::ui::use_screen_navigator();
  client::ui::internal::DeferredUiMutationSink mutations =
      client::ui::internal::use_deferred_ui_mutations();
  return use_callback(
      [game, nav, mutations] {
        if (!nav.reset_to)
          return;
        if (game && mutations) {
          mutations.submit([game] { game->reset(); });
        }
        nav.reset_to(std::make_unique<ShooterGameScreen>());
      },
      client::ui::callback_deps(
          client::ui::callback_deps_ptr(game),
          client::ui::callback_deps_ptr(mutations.owner()),
          nav.current_entry_id));
}

} // namespace shooter
