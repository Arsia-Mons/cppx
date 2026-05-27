#include "loadout_screen.h"

#include <memory>

#include <clay.h>

#include "../../../../react.h"
#include "../../../../ui/focus/ui_focus.h"
#include "../../../../ui/primitives/button.h"
#include "../../../../ui/primitives/clay_text.h"
#include "../../../../ui/primitives/selectable.h"
#include "../../../../ui/primitives/toggle.h"
#include "../../callback_deps.h"
#include "../../client_ui.h"
#include "../../components/hud_band.h"
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
    REACT_COMPONENT_BEGIN("LoadoutScreenView") {
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
            if (selected.valid) {
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

                ::ui::ui_focus_push_scope(
                    { .id = CLAY_ID("LoadoutScope"), .modal = true });
                ::ui::ui_focus_request_initial_focus(
                    weapon_tile_id(selected_index_seed));

                CLAY({
                    .id = CLAY_ID("LoadoutRoot"),
                    .layout = {
                        .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) },
                        .padding = CLAY_PADDING_ALL(24),
                        .childGap = 18,
                        .layoutDirection = CLAY_TOP_TO_BOTTOM,
                    },
                    .backgroundColor = { 12, 20, 24, 245 },
                }) {
                    CLAY_TEXT(::ui::clay_text("Loadout"),
                              CLAY_TEXT_CONFIG(
                                  { .textColor = { 236, 246, 242, 255 }, .fontSize = 26 }));
                    CLAY({
                        .id = CLAY_ID("LoadoutTabs"),
                        .layout = {
                            .sizing = { CLAY_SIZING_FIT(0), CLAY_SIZING_FIT(0) },
                            .childGap = 10,
                            .layoutDirection = CLAY_LEFT_TO_RIGHT,
                        },
                    }) {
                        ::ui::Selectable({
                            .id = CLAY_ID("WeaponsTab"),
                            .label = "Weapons",
                            .selected = *active_tab == LOADOUT_TAB_WEAPONS,
                            .on_select =
                                [active_tab, set_selected_tile, select_weapons_tab_weapon] {
                                    if (active_tab)
                                        *active_tab = LOADOUT_TAB_WEAPONS;
                                    if (set_selected_tile)
                                        set_selected_tile(
                                            first_weapon_for_tab(LOADOUT_TAB_WEAPONS));
                                    if (select_weapons_tab_weapon)
                                        select_weapons_tab_weapon();
                                },
                        });
                        ::ui::Selectable({
                            .id = CLAY_ID("GearTab"),
                            .label = "Gear",
                            .selected = *active_tab == LOADOUT_TAB_GEAR,
                            .on_select =
                                [active_tab, set_selected_tile, select_gear_tab_weapon] {
                                    if (active_tab)
                                        *active_tab = LOADOUT_TAB_GEAR;
                                    if (set_selected_tile)
                                        set_selected_tile(
                                            first_weapon_for_tab(LOADOUT_TAB_GEAR));
                                    if (select_gear_tab_weapon)
                                        select_gear_tab_weapon();
                                },
                        });
                    }
                    CLAY({
                        .id = CLAY_ID("LoadoutBody"),
                        .layout = {
                            .sizing = { CLAY_SIZING_FIT(0), CLAY_SIZING_FIT(0) },
                            .childGap = 18,
                            .layoutDirection = CLAY_LEFT_TO_RIGHT,
                        },
                    }) {
                        CLAY({
                            .id = CLAY_ID("WeaponGrid"),
                            .layout = {
                                .sizing = { CLAY_SIZING_FIXED(400), CLAY_SIZING_FIT(0) },
                                .childGap = 10,
                                .layoutDirection = CLAY_TOP_TO_BOTTOM,
                            },
                        }) {
                            if (*active_tab == LOADOUT_TAB_WEAPONS) {
                                for (int row = 0; row < 2; ++row) {
                                    CLAY({
                                        .id = CLAY_IDI("WeaponGridRow", row),
                                        .layout = {
                                            .sizing = { CLAY_SIZING_FIT(0),
                                                        CLAY_SIZING_FIT(0) },
                                            .childGap = 10,
                                            .layoutDirection = CLAY_LEFT_TO_RIGHT,
                                        },
                                    }) {
                                        WeaponTile(row * 2);
                                        if (row == 0) {
                                            WeaponTile(row * 2 + 1);
                                        }
                                    }
                                }
                            } else {
                                CLAY({
                                    .id = CLAY_ID("GearGridRow"),
                                    .layout = {
                                        .sizing = { CLAY_SIZING_FIT(0), CLAY_SIZING_FIT(0) },
                                        .childGap = 10,
                                        .layoutDirection = CLAY_LEFT_TO_RIGHT,
                                    },
                                }) {
                                    WeaponTile(3);
                                }
                            }
                        }
                        CLAY({
                            .id = CLAY_ID("LoadoutDetails"),
                            .layout = {
                                .sizing = { CLAY_SIZING_FIXED(260), CLAY_SIZING_FIT(0) },
                                .padding = CLAY_PADDING_ALL(14),
                                .childGap = 10,
                                .layoutDirection = CLAY_TOP_TO_BOTTOM,
                            },
                            .backgroundColor = { 22, 30, 36, 255 },
                            .cornerRadius = CLAY_CORNER_RADIUS(4),
                        }) {
                            CLAY_TEXT(::ui::clay_text(details),
                                      CLAY_TEXT_CONFIG({ .textColor = { 226, 238, 236, 255 },
                                                         .fontSize = 14 }));
                            ::ui::Toggle({
                                .id = CLAY_ID("CompareToggle"),
                                .label = "Compare",
                                .checked = use_compare_enabled(),
                                .on_change = use_set_compare_enabled(),
                            });
                            ::ui::Button({
                                .id = CLAY_ID("BuyWeaponButton"),
                                .label = "Buy",
                                .disabled = !can_buy,
                                .on_confirm =
                                    [set_pending, pending, selected_index_seed] {
                                        if (set_pending) {
                                            set_pending({
                                                .action = LOADOUT_ACTION_BUY,
                                                .weapon_index = selected_index_seed,
                                                .generation = pending.generation + 1,
                                            });
                                        }
                                    },
                            });
                            ::ui::Button({
                                .id = CLAY_ID("EquipWeaponButton"),
                                .label = "Equip",
                                .disabled = !can_equip,
                                .on_confirm =
                                    [set_pending, pending, selected_index_seed] {
                                        if (set_pending) {
                                            set_pending({
                                                .action = LOADOUT_ACTION_EQUIP,
                                                .weapon_index = selected_index_seed,
                                                .generation = pending.generation + 1,
                                            });
                                        }
                                    },
                            });
                            ::ui::Button({
                                .id = CLAY_ID("BackFromLoadoutButton"),
                                .label = "Back",
                                .on_confirm = nav.pop_current,
                            });
                            CLAY_TEXT(::ui::clay_text("Equipment Slots"),
                                      CLAY_TEXT_CONFIG({ .textColor = { 202, 218, 216, 255 },
                                                         .fontSize = 14 }));
                            EquipmentSlot(CLAY_ID("PrimarySlot"), "Primary", 0);
                            EquipmentSlot(CLAY_ID("GearSlot"), "Gear", 3);
                        }
                    }
                }

                LoadoutConfirmDialog();

                ::ui::ui_focus_pop_scope();
            }
        }
    }
    REACT_COMPONENT_END();
}

void LoadoutScreen::build_ui() {
    REACT_COMPONENT_BEGIN_KEY("LoadoutScreen", entry_id()) {
        bool *compare_enabled = use_state<bool>(false);
        int *selected_index = use_state<int>(0);
        LoadoutPendingAction *pending = use_state<LoadoutPendingAction>({});
        if (compare_enabled && selected_index && pending) {
            client::ui::QueueUiWrite queue_write = client::ui::use_ui_write_queue();

            LoadoutContextValue ctx;
            ctx.compare_enabled = compare_enabled;
            ctx.selected_weapon_tile = selected_index;
            ctx.pending = pending;
            ctx.set_compare_enabled = use_callback<void(bool)>(
                [compare_enabled, queue_write](bool enabled) {
                    if (queue_write && compare_enabled) {
                        queue_write(
                            [compare_enabled, enabled] { *compare_enabled = enabled; });
                    }
                },
                client::ui::callback_deps(
                    client::ui::callback_deps_ptr(compare_enabled)));
            ctx.set_selected_weapon_tile = use_callback<void(int)>(
                [selected_index, queue_write](int index) {
                    if (queue_write && selected_index) {
                        queue_write([selected_index, index] { *selected_index = index; });
                    }
                },
                client::ui::callback_deps(
                    client::ui::callback_deps_ptr(selected_index)));
            ctx.set_pending_action = use_callback<void(LoadoutPendingAction)>(
                [pending, queue_write](LoadoutPendingAction next) {
                    if (queue_write && pending) {
                        queue_write([pending, next] { *pending = next; });
                    }
                },
                client::ui::callback_deps(client::ui::callback_deps_ptr(pending)));
            ctx.clear_pending_action = use_callback(
                [pending, queue_write] {
                    if (queue_write && pending) {
                        queue_write([pending] { pending->action = LOADOUT_ACTION_NONE; });
                    }
                },
                client::ui::callback_deps(client::ui::callback_deps_ptr(pending)));

            loadout_provider_push(&ctx);
            LoadoutScreenView();
            loadout_provider_pop();
        }
    }
    REACT_COMPONENT_END();
}

} // namespace shooter
