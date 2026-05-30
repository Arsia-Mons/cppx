#include "options_actions.h"

#include <memory>

#include "client/ui/screens/options/options_screen.h"

#include "client/ui/callback_deps.h"
#include "client/ui/app_shell/client_ui.h"
#include "react.h"

namespace shooter {

std::function<void()> use_push_options_screen() {
  client::ui::ScreenNavigator nav = client::ui::use_screen_navigator();
  return use_callback(
      [nav] {
        if (nav.push)
          nav.push(std::make_unique<OptionsScreen>());
      },
      client::ui::callback_deps(nav.current_entry_id));
}

} // namespace shooter
