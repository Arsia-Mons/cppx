#include "loadout_screen.h"

#include <memory>

#include "../../../../react.h"
#include "../../../../ui/retained/components.h"
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

static void LoadoutScreenView() {
    REACT_RETAINED_COMPONENT_BEGIN("LoadoutScreenView") {
        bool is_top = client::ui::use_screen_is_top();
        client::ui::ScreenNavigator nav = client::ui::use_screen_navigator();
        int weapon_count = use_shooter_weapon_count();
        int selected_index_seed = use_selected_weapon_tile();
        int *active_tab = use_state<int>(LOADOUT_TAB_WEAPONS);
        if (active_tab) {
            if (selected_index_seed < 0 || selected_index_seed >= weapon_count) {
                selected_index_seed = 0;
            }
            if (!weapon_in_tab(selected_index_seed, *active_tab)) {
                selected_index_seed = first_weapon_for_tab(*active_tab);
            }

            ShooterWeaponRead selected = use_weapon_read(selected_index_seed);
            if (is_top && selected.valid) {
                bool can_buy = selected.can_buy;
                bool can_equip = selected.can_equip && !selected.equipped;
                std::function<void()> select_weapons_tab_weapon =
                    use_select_weapon(first_weapon_for_tab(LOADOUT_TAB_WEAPONS));
                std::function<void()> select_gear_tab_weapon =
                    use_select_weapon(first_weapon_for_tab(LOADOUT_TAB_GEAR));
                std::function<void(int)> set_selected_tile =
                    use_set_selected_weapon_tile();
                std::function<void(LoadoutPendingAction)> set_pending =
                    use_set_pending_loadout_action();
                LoadoutPendingAction pending = use_pending_loadout_action();

                const char *details =
                    use_text_storage("%s: %s, cost %d, ammo %d", selected.name,
                                     selected.role, selected.cost, selected.ammo);
                namespace retained = ::ui::retained;
                bool confirm_open = pending.action != LOADOUT_ACTION_NONE;

                LoadoutConfirmDialog();

                retained::Panel(
                    {
                        .key = "root",
                        .width = retained::Length::percent(100.0f),
                        .height = retained::Length::percent(100.0f),
                        .padding = {24.0f, 24.0f, 24.0f, 24.0f},
                        .gap = 18.0f,
                        .modal = !confirm_open,
                        .background = {12, 20, 24, 245},
                    },
                    [&] {
                        retained::Text({
                            .key = "title",
                            .value = "Loadout",
                            .height = retained::Length::points(30.0f),
                            .text_color = {236, 246, 242, 255},
                            .font_size = 26,
                        });
                        retained::Panel(
                            {
                                .key = "tabs",
                                .direction = retained::FlexDirection::Row,
                                .align_items = retained::AlignItems::Start,
                                .gap = 10.0f,
                            },
                            [&] {
                                retained::Selectable({
                                    .key = "weapons",
                                    .id = "WeaponsTab",
                                    .label = "Weapons",
                                    .selected = *active_tab == LOADOUT_TAB_WEAPONS,
                                    .on_confirm =
                                        [active_tab, set_selected_tile,
                                         select_weapons_tab_weapon] {
                                            if (active_tab)
                                                *active_tab = LOADOUT_TAB_WEAPONS;
                                            if (set_selected_tile)
                                                set_selected_tile(first_weapon_for_tab(
                                                    LOADOUT_TAB_WEAPONS));
                                            if (select_weapons_tab_weapon)
                                                select_weapons_tab_weapon();
                                        },
                                });
                                retained::Selectable({
                                    .key = "gear",
                                    .id = "GearTab",
                                    .label = "Gear",
                                    .selected = *active_tab == LOADOUT_TAB_GEAR,
                                    .on_confirm =
                                        [active_tab, set_selected_tile,
                                         select_gear_tab_weapon] {
                                            if (active_tab)
                                                *active_tab = LOADOUT_TAB_GEAR;
                                            if (set_selected_tile)
                                                set_selected_tile(first_weapon_for_tab(
                                                    LOADOUT_TAB_GEAR));
                                            if (select_gear_tab_weapon)
                                                select_gear_tab_weapon();
                                        },
                                });
                            });
                        retained::Panel(
                            {
                                .key = "body",
                                .direction = retained::FlexDirection::Row,
                                .align_items = retained::AlignItems::Start,
                                .gap = 18.0f,
                            },
                            [&] {
                                retained::Panel(
                                    {
                                        .key = "weapon-grid",
                                        .width = retained::Length::points(400.0f),
                                        .gap = 10.0f,
                                    },
                                    [&] {
                                        if (*active_tab == LOADOUT_TAB_WEAPONS) {
                                            for (int row = 0; row < 2; ++row) {
                                                retained::Panel(
                                                    {
                                                        .key = row == 0
                                                                   ? "weapon-row-0"
                                                                   : "weapon-row-1",
                                                        .direction =
                                                            retained::FlexDirection::Row,
                                                        .align_items =
                                                            retained::AlignItems::Start,
                                                        .gap = 10.0f,
                                                    },
                                                    [row] {
                                                        WeaponTile(row * 2);
                                                        if (row == 0) {
                                                            WeaponTile(row * 2 + 1);
                                                        }
                                                    });
                                            }
                                        } else {
                                            retained::Panel(
                                                {
                                                    .key = "gear-row",
                                                    .direction =
                                                        retained::FlexDirection::Row,
                                                    .align_items =
                                                        retained::AlignItems::Start,
                                                    .gap = 10.0f,
                                                },
                                                [] {
                                                    WeaponTile(3);
                                                });
                                        }
                                    });
                                retained::Panel(
                                    {
                                        .key = "details",
                                        .width = retained::Length::points(260.0f),
                                        .padding = {14.0f, 14.0f, 14.0f, 14.0f},
                                        .gap = 10.0f,
                                        .background = {22, 30, 36, 255},
                                    },
                                    [&] {
                                        retained::Text({
                                            .key = "summary",
                                            .value = details,
                                            .height =
                                                retained::Length::points(18.0f),
                                            .text_color = {226, 238, 236, 255},
                                            .font_size = 14,
                                        });
                                        retained::Toggle({
                                            .key = "compare",
                                            .id = "CompareToggle",
                                            .label = "Compare",
                                            .checked = use_compare_enabled(),
                                            .on_change = use_set_compare_enabled(),
                                        });
                                        retained::Button({
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
                                                                pending.generation + 1,
                                                        });
                                                    }
                                                },
                                        });
                                        retained::Button({
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
                                                                pending.generation + 1,
                                                        });
                                                    }
                                                },
                                        });
                                        retained::Button({
                                            .key = "back",
                                            .id = "BackFromLoadoutButton",
                                            .label = "Back",
                                            .on_confirm = nav.pop_current,
                                        });
                                        retained::Text({
                                            .key = "slots-title",
                                            .value = "Equipment Slots",
                                            .height =
                                                retained::Length::points(16.0f),
                                            .text_color = {202, 218, 216, 255},
                                            .font_size = 14,
                                        });
                                        EquipmentSlot("PrimarySlot", "Primary", 0);
                                        EquipmentSlot("GearSlot", "Gear", 3);
                                    });
                            });
                    });
            }
        }
    } REACT_RETAINED_COMPONENT_END();
}

void LoadoutScreen::build_ui() {
    REACT_RETAINED_COMPONENT_BEGIN_KEY("LoadoutScreen", entry_id()) {
        bool *compare_enabled = use_state<bool>(false);
        int *selected_index = use_state<int>(0);
        LoadoutPendingAction *pending = use_state<LoadoutPendingAction>({});
        if (compare_enabled && selected_index && pending) {
            LoadoutContextValue ctx =
                use_loadout_context_value(compare_enabled, selected_index, pending);

            loadout_provider_push(&ctx);
            LoadoutScreenView();
            loadout_provider_pop();
        }
    } REACT_RETAINED_COMPONENT_END();
}

} // namespace shooter
