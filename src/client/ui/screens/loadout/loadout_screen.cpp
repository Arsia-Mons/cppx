#include "loadout_screen.h"

#include <memory>
#include <stdio.h>

#include "../../../../react.h"
#include "../../../../ui/components/components.h"
#include "../../callback_deps.h"
#include "../../client_ui.h"
#include "../../hooks/shooter_weapons.h"
#include "../../providers/shooter_provider.h"
#include "components/confirm_dialog.h"
#include "components/equipment_slot.h"
#include "components/weapon_tile.h"
#include "loadout_state.h"

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

namespace {

struct LoadoutScreenProps {
  uint32_t unused = 0;
};

struct LoadoutScreenBodyProps {
  uint32_t unused = 0;
};

const char *screen_entry_key(const char *prefix,
                             client::ui::UiScreenEntryId entry_id) {
  char key[64] = {};
  snprintf(key, sizeof(key), "%s-%u", prefix, entry_id);
  return ::ui::copy_string(key);
}

::ui::UiChildren weapon_grid_children(int active_tab) {
  namespace components = ::ui::components;
  if (active_tab == LOADOUT_TAB_GEAR) {
    return ::ui::children({
        ::ui::components::elements::Box({
            .key = "gear-row",
            .style =
                {
                    .direction = ::ui::FlexDirection::Row,
                    .align_items = ::ui::AlignItems::Start,
                    .gap = 10.0f,
            },
            .children = ::ui::children({
                ::ui::component("WeaponTile",
                                WeaponTileProps{
                                    .key = weapon_tile_key(3),
                                    .index = 3,
                                },
                                WeaponTile),
            }),
        }),
    });
  }

  return ::ui::children({
      ::ui::components::elements::Box({
          .key = "weapon-row-0",
          .style =
              {
                  .direction = ::ui::FlexDirection::Row,
                  .align_items = ::ui::AlignItems::Start,
                  .gap = 10.0f,
          },
          .children = ::ui::children({
              ::ui::component("WeaponTile",
                              WeaponTileProps{
                                  .key = weapon_tile_key(0),
                                  .index = 0,
                              },
                              WeaponTile),
              ::ui::component("WeaponTile",
                              WeaponTileProps{
                                  .key = weapon_tile_key(1),
                                  .index = 1,
                              },
                              WeaponTile),
          }),
      }),
      ::ui::components::elements::Box({
          .key = "weapon-row-1",
          .style =
              {
                  .direction = ::ui::FlexDirection::Row,
                  .align_items = ::ui::AlignItems::Start,
                  .gap = 10.0f,
          },
          .children = ::ui::children({
              ::ui::component("WeaponTile",
                              WeaponTileProps{
                                  .key = weapon_tile_key(2),
                                  .index = 2,
                              },
                              WeaponTile),
          }),
      }),
  });
}

::ui::UiElement
LoadoutScreenBody(const LoadoutScreenBodyProps &props) {
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
  namespace components = ::ui::components;

  if (!is_top || !selected.valid)
    return ::ui::empty();

  bool confirm_open = pending.action != LOADOUT_ACTION_NONE;
  return ::ui::fragment(
      ::ui::children(
          {
              ::ui::component("LoadoutConfirmDialog",
                              LoadoutConfirmDialogProps{},
                              LoadoutConfirmDialog),
              ::ui::components::elements::Dialog(
                  {
                      .key = "root",
                      .modal = !confirm_open,
                      .style =
                          {
                              .width = ::ui::Length::percent(100.0f),
                              .height = ::ui::Length::percent(100.0f),
                              .padding = {24.0f, 24.0f, 24.0f, 24.0f},
                              .gap = 18.0f,
                              .background = {12, 20, 24, 245},
                          },
                      .children =
                          ::ui::children(
                              {
                                  ::ui::components::elements::Text(
                                      {
                                          .key = "title",
                                          .value = "Loadout",
                                          .style =
                                              {
                                                  .height =
                                                      ::ui::Length::points(
                                                          30.0f),
                                                  .text = {236, 246, 242, 255},
                                                  .font_size = 26,
                                              },
                                      }),
                                  ::ui::components::elements::Box(
                                      {
                                          .key = "tabs",
                                          .style =
                                              {
                                                  .direction =
                                                      ::ui::FlexDirection::Row,
                                                  .align_items =
                                                      ::ui::AlignItems::Start,
                                                  .gap = 10.0f,
                                              },
                                          .children =
                                              ::ui::children(
                                                  {
                                                      ::ui::components::elements::Button({
                                                          .key = "weapons",
                                                          .id = "WeaponsTab",
                                                          .accessibility =
                                                              {
                                                                  .role = ::
                                                                      ui::SemanticRole::
                                                                          Tab,
                                                              },
                                                          .label = "Weapons",
                                                          .on_activate =
                                                              [active_tab,
                                                               set_selected_tile,
                                                               select_weapons_tab_weapon](
                                                                  const ::ui::
                                                                      ActivationEvent
                                                                          &) {
                                                                if (active_tab)
                                                                  *active_tab =
                                                                      LOADOUT_TAB_WEAPONS;
                                                                if (set_selected_tile)
                                                                  set_selected_tile(
                                                                      first_weapon_for_tab(
                                                                          LOADOUT_TAB_WEAPONS));
                                                                if (select_weapons_tab_weapon)
                                                                  select_weapons_tab_weapon();
                                                              },
                                                          .style =
                                                              {
                                                                  .align_items =
                                                                      ::ui::AlignItems::Center,
                                                                  .justify_content =
                                                                      ::
                                                                          ui::JustifyContent::
                                                                              Center,
                                                                  .width =
                                                                      ::
                                                                          ui::Length::points(
                                                                              132.0f),
                                                                  .height =
                                                                      ::ui::Length::points(34.0f),
                                                                  .padding =
                                                                      {12.0f,
                                                                       12.0f,
                                                                       7.0f,
                                                                       7.0f},
                                                                  .background =
                                                                      *active_tab ==
                                                                              LOADOUT_TAB_WEAPONS
                                                                          ? ::ui::Color{42, 80, 60, 255}
                                                                          : ::ui::Color{24, 28, 36, 255},
                                                              },
                                                      }),
                                                      ::ui::components::elements::Button(
                                                          {
                                                              .key = "gear",
                                                              .id = "GearTab",
                                                              .accessibility =
                                                                  {
                                                                      .role = ::
                                                                          ui::SemanticRole::
                                                                              Tab,
                                                                  },
                                                              .label = "Gear",
                                                              .on_activate =
                                                                  [active_tab,
                                                                   set_selected_tile,
                                                                   select_gear_tab_weapon](
                                                                      const ::ui::
                                                                          ActivationEvent
                                                                              &) {
                                                                    if (active_tab)
                                                                      *active_tab =
                                                                          LOADOUT_TAB_GEAR;
                                                                    if (set_selected_tile)
                                                                      set_selected_tile(
                                                                          first_weapon_for_tab(
                                                                              LOADOUT_TAB_GEAR));
                                                                    if (select_gear_tab_weapon)
                                                                      select_gear_tab_weapon();
                                                                  },
                                                              .style =
                                                                  {
                                                                      .align_items =
                                                                          ::ui::AlignItems::Center,
                                                                      .justify_content =
                                                                          ::
                                                                              ui::JustifyContent::
                                                                                  Center,
                                                                      .width =
                                                                          ::
                                                                              ui::Length::points(
                                                                                  132.0f),
                                                                      .height =
                                                                          ::ui::Length::points(34.0f),
                                                                      .padding =
                                                                          {12.0f,
                                                                           12.0f,
                                                                           7.0f,
                                                                           7.0f},
                                                                      .background =
                                                                          *active_tab ==
                                                                                  LOADOUT_TAB_GEAR
                                                                              ? ::ui::Color{42, 80, 60, 255}
                                                                              : ::ui::Color{24, 28, 36, 255},
                                                                  },
                                                          }),
                                                  }),
                                      }),
                                  ::ui::components::elements::Box(
                                      {
                                          .key = "body",
                                          .style =
                                              {
                                                  .direction =
                                                      ::ui::FlexDirection::Row,
                                                  .align_items =
                                                      ::ui::AlignItems::Start,
                                                  .gap = 18.0f,
                                              },
                                          .children =
                                              ::ui::children(
                                                  {
                                                      ::ui::components::elements::Box({
                                                          .key = "weapon-grid",
                                                          .style =
                                                              {
                                                                  .width =
                                                                      ::
                                                                          ui::Length::points(
                                                                              400.0f),
                                                                  .gap = 10.0f,
                                                              },
                                                          .children =
                                                              weapon_grid_children(
                                                                  *active_tab),
                                                      }),
                                                      ::ui::components::elements::Box(
                                                          {
                                                              .key = "details",
                                                              .style =
                                                                  {
                                                                      .width =
                                                                          ::
                                                                              ui::Length::points(
                                                                                  260.0f),
                                                                      .padding =
                                                                          {14.0f,
                                                                           14.0f,
                                                                           14.0f,
                                                                           14.0f},
                                                                      .gap =
                                                                          10.0f,
                                                                      .background =
                                                                          {22,
                                                                           30, 36, 255},
                                                                  },
                                                              .children = ::ui::
                                                                  children({
                                                                      ::ui::components::elements::Text({
                                                                          .key =
                                                                              "summary",
                                                                          .value =
                                                                              details,
                                                                          .style =
                                                                              {
                                                                                  .height =
                                                                                      ::ui::Length::points(18.0f),
                                                                                  .text = {226, 238, 236, 255},
                                                                                  .font_size =
                                                                                      14,
                                                                              },
                                                                      }),
                                                                      ::ui::components::elements::Checkbox({
                                                                          .key =
                                                                              "compare",
                                                                          .id =
                                                                              "CompareToggle",
                                                                          .checked =
                                                                              compare_enabled,
                                                                          .label =
                                                                              "Compare",
                                                                          .on_change =
                                                                              set_compare_enabled,
                                                                      }),
                                                                      ::ui::components::elements::Button({
                                                                          .key =
                                                                              "buy",
                                                                          .id =
                                                                              "BuyWeaponButton",
                                                                          .disabled =
                                                                              !can_buy,
                                                                          .label =
                                                                              "Buy",
                                                                          .on_activate =
                                                                              [set_pending,
                                                                               pending,
                                                                               selected_index_seed](
                                                                                  const ::ui::ActivationEvent
                                                                                      &) {
                                                                                if (set_pending) {
                                                                                  set_pending({
                                                                                      .action =
                                                                                          LOADOUT_ACTION_BUY,
                                                                                      .weapon_index =
                                                                                          selected_index_seed,
                                                                                      .generation =
                                                                                          pending
                                                                                              .generation +
                                                                                          1,
                                                                                  });
                                                                                }
                                                                              },
                                                                      }),
                                                                      ::ui::components::elements::Button({
                                                                          .key =
                                                                              "equip",
                                                                          .id =
                                                                              "EquipWeaponButton",
                                                                          .disabled =
                                                                              !can_equip,
                                                                          .label =
                                                                              "Equip",
                                                                          .on_activate =
                                                                              [set_pending,
                                                                               pending,
                                                                               selected_index_seed](
                                                                                  const ::ui::
                                                                                      ActivationEvent &) {
                                                                                if (set_pending) {
                                                                                  set_pending({
                                                                                      .action =
                                                                                          LOADOUT_ACTION_EQUIP,
                                                                                      .weapon_index =
                                                                                          selected_index_seed,
                                                                                      .generation =
                                                                                          pending
                                                                                              .generation +
                                                                                          1,
                                                                                  });
                                                                                }
                                                                              },
                                                                      }),
                                                                      ::ui::components::elements::Button({
                                                                          .key =
                                                                              "back",
                                                                          .id =
                                                                              "BackFromLoadoutButton",
                                                                          .label =
                                                                              "Back",
                                                                          .on_activate =
                                                                              [pop =
                                                                                   nav.pop_current](
                                                                                  const ::ui::ActivationEvent
                                                                                      &) {
                                                                                if (pop)
                                                                                  pop();
                                                                              },
                                                                      }),
                                                                      ::ui::components::elements::Text({
                                                                          .key =
                                                                              "slots-title",
                                                                          .value =
                                                                              "Equipment Slots",
                                                                          .style =
                                                                              {
                                                                                  .height =
                                                                                      ::ui::Length::points(16.0f),
                                                                                  .text = {202, 218, 216, 255},
                                                                                  .font_size =
                                                                                      14,
                                                                              },
                                                                      }),
                                                                      ::ui::component(
                                                                          "EquipmentSlot",
                                                                          EquipmentSlotProps{
                                                                              .key =
                                                                                  "PrimarySlot",
                                                                              .id =
                                                                                  "PrimarySlot",
                                                                              .label =
                                                                                  "Primary",
                                                                              .weapon_index =
                                                                                  0,
                                                                          },
                                                                          EquipmentSlot),
                                                                      ::ui::component(
                                                                          "EquipmentSlot",
                                                                          EquipmentSlotProps{
                                                                              .key =
                                                                                  "GearSlot",
                                                                              .id =
                                                                                  "GearSlot",
                                                                              .label =
                                                                                  "Gear",
                                                                              .weapon_index =
                                                                                  3,
                                                                          },
                                                                          EquipmentSlot),
                                                                  }),
                                                          }),
                                                  }),
                                      }),
                              }),
                  }),
          }));
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
