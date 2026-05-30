#include "client/ui/screens/loadout/loadout_actions.h"

#include "client/ui/screens/loadout/loadout_screen.h"
#include "client/ui/client_ui.h"
#include "client/ui/callback_deps.h"
#include "react.h"

#include <memory>

namespace shooter {

std::function<void()> use_push_loadout_screen() {
  client::ui::ScreenNavigator nav = client::ui::use_screen_navigator();
  return use_callback(
      [nav] {
        if (nav.push)
          nav.push(std::make_unique<LoadoutScreen>());
      },
      client::ui::callback_deps(nav.current_entry_id));
}

} // namespace shooter
