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

const char *screen_entry_key(::ui::retained::UiElementFrame &frame,
                             const char *prefix,
                             client::ui::UiScreenEntryId entry_id) {
  char key[64] = {};
  snprintf(key, sizeof(key), "%s-%u", prefix, entry_id);
  return frame.copy_string(key);
}

::ui::retained::UiChildren
weapon_grid_children(::ui::retained::UiElementFrame &frame, int active_tab) {
  namespace retained = ::ui::retained;
  namespace components = ::ui::components;
  if (active_tab == LOADOUT_TAB_GEAR) {
    return frame.children({
        components::Box(frame,
                        {
                            .key = "gear-row",
                            .direction = retained::FlexDirection::Row,
                            .align_items = retained::AlignItems::Start,
                            .gap = 10.0f,
                            .children = frame.children({
                                WeaponTile(frame, 3),
                            }),
                        }),
    });
  }

  return frame.children({
      components::Box(frame,
                      {
                          .key = "weapon-row-0",
                          .direction = retained::FlexDirection::Row,
                          .align_items = retained::AlignItems::Start,
                          .gap = 10.0f,
                          .children = frame.children({
                              WeaponTile(frame, 0),
                              WeaponTile(frame, 1),
                          }),
                      }),
      components::Box(frame,
                      {
                          .key = "weapon-row-1",
                          .direction = retained::FlexDirection::Row,
                          .align_items = retained::AlignItems::Start,
                          .gap = 10.0f,
                          .children = frame.children({
                              WeaponTile(frame, 2),
                          }),
                      }),
  });
}

::ui::retained::UiElement
render_loadout_screen_body(const LoadoutScreenBodyProps &props,
                           ::ui::retained::UiElementFrame &frame) {
  (void)props;
  bool is_top = client::ui::use_screen_is_top();
  client::ui::ScreenNavigator nav = client::ui::use_screen_navigator();
  int weapon_count = use_shooter_weapon_count();
  int selected_index_seed = use_selected_weapon_tile();
  int *active_tab = use_state<int>(LOADOUT_TAB_WEAPONS);
  if (!active_tab)
    return frame.empty();

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
  namespace retained = ::ui::retained;
  namespace components = ::ui::components;

  if (!is_top || !selected.valid)
    return frame.empty();

  bool confirm_open = pending.action != LOADOUT_ACTION_NONE;
  return frame.fragment(frame.children({
      LoadoutConfirmDialog(frame),
      components::Box(
          frame,
          {
              .key = "root",
              .width = retained::Length::percent(100.0f),
              .height = retained::Length::percent(100.0f),
              .padding = {24.0f, 24.0f, 24.0f, 24.0f},
              .gap = 18.0f,
              .modal = !confirm_open,
              .background = {12, 20, 24, 245},
              .children = frame.children({
                  components::Text(
                      frame,
                      {
                          .key = "title",
                          .value = "Loadout",
                          .height = retained::Length::points(30.0f),
                          .text_color = {236, 246, 242, 255},
                          .font_size = 26,
                      }),
                  components::Box(
                      frame,
                      {
                          .key = "tabs",
                          .direction = retained::FlexDirection::Row,
                          .align_items = retained::AlignItems::Start,
                          .gap = 10.0f,
                          .children = frame.children({
                              components::Selectable(
                                  frame,
                                  {
                                      .key = "weapons",
                                      .id = "WeaponsTab",
                                      .label = "Weapons",
                                      .selected = *active_tab ==
                                                  LOADOUT_TAB_WEAPONS,
                                      .on_confirm =
                                          [active_tab, set_selected_tile,
                                           select_weapons_tab_weapon] {
                                            if (active_tab)
                                              *active_tab = LOADOUT_TAB_WEAPONS;
                                            if (set_selected_tile)
                                              set_selected_tile(
                                                  first_weapon_for_tab(
                                                      LOADOUT_TAB_WEAPONS));
                                            if (select_weapons_tab_weapon)
                                              select_weapons_tab_weapon();
                                          },
                                  }),
                              components::Selectable(
                                  frame,
                                  {
                                      .key = "gear",
                                      .id = "GearTab",
                                      .label = "Gear",
                                      .selected = *active_tab ==
                                                  LOADOUT_TAB_GEAR,
                                      .on_confirm =
                                          [active_tab, set_selected_tile,
                                           select_gear_tab_weapon] {
                                            if (active_tab)
                                              *active_tab = LOADOUT_TAB_GEAR;
                                            if (set_selected_tile)
                                              set_selected_tile(
                                                  first_weapon_for_tab(
                                                      LOADOUT_TAB_GEAR));
                                            if (select_gear_tab_weapon)
                                              select_gear_tab_weapon();
                                          },
                                  }),
                          }),
                      }),
                  components::Box(
                      frame,
                      {
                          .key = "body",
                          .direction = retained::FlexDirection::Row,
                          .align_items = retained::AlignItems::Start,
                          .gap = 18.0f,
                          .children = frame.children({
                              components::Box(
                                  frame,
                                  {
                                      .key = "weapon-grid",
                                      .width = retained::Length::points(400.0f),
                                      .gap = 10.0f,
                                      .children = weapon_grid_children(
                                          frame, *active_tab),
                                  }),
                              components::Box(
                                  frame,
                                  {
                                      .key = "details",
                                      .width = retained::Length::points(260.0f),
                                      .padding = {14.0f, 14.0f, 14.0f, 14.0f},
                                      .gap = 10.0f,
                                      .background = {22, 30, 36, 255},
                                      .children = frame.children({
                                          components::Text(
                                              frame,
                                              {
                                                  .key = "summary",
                                                  .value = details,
                                                  .height =
                                                      retained::Length::points(
                                                          18.0f),
                                                  .text_color = {226, 238, 236,
                                                                 255},
                                                  .font_size = 14,
                                              }),
                                          components::Toggle(
                                              frame,
                                              {
                                                  .key = "compare",
                                                  .id = "CompareToggle",
                                                  .label = "Compare",
                                                  .checked = compare_enabled,
                                                  .on_change =
                                                      set_compare_enabled,
                                              }),
                                          components::Button(
                                              frame,
                                              {
                                                  .key = "buy",
                                                  .id = "BuyWeaponButton",
                                                  .label = "Buy",
                                                  .disabled = !can_buy,
                                                  .on_confirm =
                                                      [set_pending, pending,
                                                       selected_index_seed] {
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
                                          components::Button(
                                              frame,
                                              {
                                                  .key = "equip",
                                                  .id = "EquipWeaponButton",
                                                  .label = "Equip",
                                                  .disabled = !can_equip,
                                                  .on_confirm =
                                                      [set_pending, pending,
                                                       selected_index_seed] {
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
                                          components::Button(
                                              frame,
                                              {
                                                  .key = "back",
                                                  .id = "BackFromLoadoutButton",
                                                  .label = "Back",
                                                  .on_confirm = nav.pop_current,
                                              }),
                                          components::Text(
                                              frame,
                                              {
                                                  .key = "slots-title",
                                                  .value = "Equipment Slots",
                                                  .height =
                                                      retained::Length::points(
                                                          16.0f),
                                                  .text_color = {202, 218, 216,
                                                                 255},
                                                  .font_size = 14,
                                              }),
                                          EquipmentSlot(frame, "PrimarySlot",
                                                        "Primary", 0),
                                          EquipmentSlot(frame, "GearSlot",
                                                        "Gear", 3),
                                      }),
                                  }),
                          }),
                      }),
              }),
          }),
  }));
}

::ui::retained::UiElement
render_loadout_screen(const LoadoutScreenProps &props,
                      ::ui::retained::UiElementFrame &frame) {
  (void)props;
  bool *compare_enabled = use_state<bool>(false);
  int *selected_index = use_state<int>(0);
  LoadoutPendingAction *pending = use_state<LoadoutPendingAction>({});
  if (!compare_enabled || !selected_index || !pending)
    return frame.empty();

  LoadoutContextValue ctx =
      use_loadout_context_value(compare_enabled, selected_index, pending);
  return LoadoutProvider(
      frame, ctx,
      frame.children({
          frame.component("LoadoutScreenBody", LoadoutScreenBodyProps{},
                          render_loadout_screen_body),
      }));
}

} // namespace

bool LoadoutScreen::build_element(::ui::retained::UiElementFrame &frame,
                                  ::ui::retained::UiElement *out) {
  if (!out)
    return false;
  *out = frame.component("LoadoutScreen", LoadoutScreenProps{},
                         render_loadout_screen,
                         screen_entry_key(frame, "loadout", entry_id()));
  return true;
}

void LoadoutScreen::build_ui() {}

} // namespace shooter
