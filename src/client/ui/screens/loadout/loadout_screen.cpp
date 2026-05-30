#include "loadout_screen.h"

#include <stdio.h>

#include "../../../../react.h"
#include "../../../../ui/runtime/element.h"
#include "client/ui/screens/loadout/components/loadout_content.h"
#include "loadout_state.h"

namespace shooter {

namespace {

struct LoadoutScreenProps {
  uint32_t unused = 0;
};

const char *screen_entry_key(const char *prefix,
                             client::ui::UiScreenEntryId entry_id) {
  char key[64] = {};
  snprintf(key, sizeof(key), "%s-%u", prefix, entry_id);
  return ::ui::copy_string(key);
}

::ui::UiElement LoadoutScreenView(const LoadoutScreenProps &props) {
  (void)props;
  bool *compare_enabled = use_state<bool>(false);
  int *selected_index = use_state<int>(0);
  LoadoutPendingAction *pending = use_state<LoadoutPendingAction>({});
  if (!compare_enabled || !selected_index || !pending)
    return ::ui::empty();

  LoadoutContextValue ctx =
      use_loadout_context_value(compare_enabled, selected_index, pending);
  return LoadoutProvider(
      ctx, ::ui::children({
               ::ui::component("LoadoutScreenBody", LoadoutScreenBodyProps{},
                               LoadoutScreenBody),
           }));
}

} // namespace

bool LoadoutScreen::build_element(::ui::UiElementFrame &frame,
                                  ::ui::UiElement *out) {
  if (!out)
    return false;
  *out = ::ui::component("LoadoutScreen", LoadoutScreenProps{},
                         LoadoutScreenView,
                         screen_entry_key("loadout", entry_id()));
  return true;
}

void LoadoutScreen::build_ui() {}

} // namespace shooter
