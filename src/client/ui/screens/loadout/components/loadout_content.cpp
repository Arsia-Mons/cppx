#include "client/ui/screens/loadout/components/loadout_content.h"

#include <functional>

#include "react.h"
#include "ui/components/common.h"
#include "client/ui/client_ui.h"
#include "client/ui/hooks/shooter_weapons.h"
#include "client/ui/providers/shooter_provider.h"
#include "client/ui/screens/loadout/components/confirm_dialog.h"
#include "client/ui/screens/loadout/components/loadout_body.h"
#include "client/ui/screens/loadout/components/loadout_details.h"
#include "client/ui/screens/loadout/components/loadout_screen_frame.h"
#include "client/ui/screens/loadout/components/loadout_tabs.h"
#include "client/ui/screens/loadout/components/loadout_title.h"
#include "client/ui/screens/loadout/components/loadout_weapon_grid.h"
#include "client/ui/screens/loadout/components/weapon_tile.h"
#include "client/ui/screens/loadout/loadout_state.h"

namespace shooter {

::ui::UiElement LoadoutScreenBody(const LoadoutScreenBodyProps &props) {
  (void)props;
  bool is_top = client::ui::use_screen_is_top();
  client::ui::ScreenNavigator nav = client::ui::use_screen_navigator();
  int weapon_count = use_shooter_weapon_count();
  int selected_index_seed = use_selected_weapon_tile();
  int *active_tab = use_state<int>(LOADOUT_TAB_WEAPONS);
  if (!active_tab)
    return ::ui::empty();

  if (selected_index_seed < 0 || selected_index_seed >= weapon_count) {
    selected_index_seed = 0;
  }
  if (!weapon_in_tab(selected_index_seed, *active_tab)) {
    selected_index_seed = first_weapon_for_tab(*active_tab);
  }

  ShooterWeaponRead selected = use_weapon_read(selected_index_seed);
  bool can_buy = selected.valid && selected.can_buy;
  bool can_equip = selected.valid && selected.can_equip && !selected.equipped;
  std::function<void()> select_weapons_tab_weapon =
      use_select_weapon(first_weapon_for_tab(LOADOUT_TAB_WEAPONS));
  std::function<void()> select_gear_tab_weapon =
      use_select_weapon(first_weapon_for_tab(LOADOUT_TAB_GEAR));
  std::function<void(int)> set_selected_tile = use_set_selected_weapon_tile();
  std::function<void(LoadoutPendingAction)> set_pending =
      use_set_pending_loadout_action();
  LoadoutPendingAction pending = use_pending_loadout_action();
  const char *details =
      use_text_storage("%s: %s, cost %d, ammo %d", selected.name, selected.role,
                       selected.cost, selected.ammo);
  bool compare_enabled = use_compare_enabled();
  std::function<void(bool)> set_compare_enabled = use_set_compare_enabled();

  if (!is_top || !selected.valid)
    return ::ui::empty();

  bool confirm_open = pending.action != LOADOUT_ACTION_NONE;

  // Tab on_select closures: write #1 (*active_tab) is synchronous local state;
  // writes #2 (set_selected_tile) and #3 (select_*_tab_weapon) are the two
  // deferred writes the GearTab contract pins. All three live here so the gear
  // tab still queues exactly two deferred mutations.
  std::function<void()> weapons_on_select =
      [active_tab, set_selected_tile, select_weapons_tab_weapon] {
        if (active_tab)
          *active_tab = LOADOUT_TAB_WEAPONS;
        if (set_selected_tile)
          set_selected_tile(first_weapon_for_tab(LOADOUT_TAB_WEAPONS));
        if (select_weapons_tab_weapon)
          select_weapons_tab_weapon();
      };
  std::function<void()> gear_on_select =
      [active_tab, set_selected_tile, select_gear_tab_weapon] {
        if (active_tab)
          *active_tab = LOADOUT_TAB_GEAR;
        if (set_selected_tile)
          set_selected_tile(first_weapon_for_tab(LOADOUT_TAB_GEAR));
        if (select_gear_tab_weapon)
          select_gear_tab_weapon();
      };

  std::function<void()> on_buy = [set_pending, pending, selected_index_seed] {
    if (set_pending) {
      set_pending({
          .action = LOADOUT_ACTION_BUY,
          .weapon_index = selected_index_seed,
          .generation = pending.generation + 1,
      });
    }
  };
  std::function<void()> on_equip = [set_pending, pending, selected_index_seed] {
    if (set_pending) {
      set_pending({
          .action = LOADOUT_ACTION_EQUIP,
          .weapon_index = selected_index_seed,
          .generation = pending.generation + 1,
      });
    }
  };
  std::function<void()> on_back = [pop = nav.pop_current] {
    if (pop)
      pop();
  };

  return ::ui::fragment(::ui::children({
      ::ui::component("LoadoutConfirmDialog", LoadoutConfirmDialogProps{},
                      LoadoutConfirmDialog),
      ::ui::component(
          "LoadoutScreenFrame",
          LoadoutScreenFrameProps{
              .key = "root",
              .confirm_open = confirm_open,
              .children = ::ui::children({
                  ::ui::component("LoadoutTitle",
                                  LoadoutTitleProps{
                                      .key = "title",
                                      .value = "Loadout",
                                  },
                                  LoadoutTitle),
                  ::ui::component(
                      "LoadoutTabs",
                      LoadoutTabsProps{
                          .key = "tabs",
                          .children = ::ui::children({
                              ::ui::component(
                                  "LoadoutTabs.Tab",
                                  LoadoutTabs::TabProps{
                                      .key = "weapons",
                                      .control_id = "WeaponsTab",
                                      .label = "Weapons",
                                      .on_select = weapons_on_select,
                                  },
                                  LoadoutTabs::Tab),
                              ::ui::component(
                                  "LoadoutTabs.Tab",
                                  LoadoutTabs::TabProps{
                                      .key = "gear",
                                      .control_id = "GearTab",
                                      .label = "Gear",
                                      .on_select = gear_on_select,
                                  },
                                  LoadoutTabs::Tab),
                          }),
                      },
                      LoadoutTabs),
                  ::ui::component(
                      "LoadoutBody",
                      LoadoutBodyProps{
                          .key = "body",
                          .children = ::ui::children({
                              ::ui::component("LoadoutWeaponGrid",
                                              LoadoutWeaponGridProps{
                                                  .key = "weapon-grid",
                                                  .active_tab = *active_tab,
                                              },
                                              LoadoutWeaponGrid),
                              ::ui::component(
                                  "LoadoutDetails",
                                  LoadoutDetailsProps{
                                      .key = "details",
                                      .summary = details,
                                      .can_buy = can_buy,
                                      .can_equip = can_equip,
                                      .compare_enabled = compare_enabled,
                                      .on_compare_change = set_compare_enabled,
                                      .on_buy = on_buy,
                                      .on_equip = on_equip,
                                      .on_back = on_back,
                                  },
                                  LoadoutDetails),
                          }),
                      },
                      LoadoutBody),
              }),
          },
          LoadoutScreenFrame),
  }));
}

} // namespace shooter
