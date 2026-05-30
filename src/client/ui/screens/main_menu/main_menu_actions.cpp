#include "main_menu_actions.h"

#include <memory>

#include "client/ui/callback_deps.h"
#include "client/ui/client_ui.h"
#include "client/ui/screens/main_menu/main_menu_screen.h"
#include "react.h"

namespace shooter {

std::function<void()> use_exit_to_main_menu() {
  client::ui::ScreenNavigator nav = client::ui::use_screen_navigator();
  return use_callback(
      [nav] {
        if (nav.reset_to)
          nav.reset_to(std::make_unique<MainMenuScreen>());
      },
      client::ui::callback_deps(nav.current_entry_id));
}

} // namespace shooter
