#include "loadout_screen.h"

#include <memory>

#include <clay.h>

#include "components/confirm_dialog.h"
#include "components/equipment_slot.h"
#include "components/weapon_tile.h"
#include "../../client_ui.h"
#include "../../components/hud_band.h"
#include "../../hooks/shooter_weapons.h"
#include "../../providers/shooter_provider.h"
#include "../../../../react.h"
#include "../../../../ui/focus/ui_focus.h"
#include "../../../../ui/primitives/button.h"
#include "../../../../ui/primitives/clay_text.h"
#include "../../../../ui/primitives/selectable.h"
#include "../../../../ui/primitives/toggle.h"

#include <stdio.h>

namespace shooter {

static ReactContext LoadoutScreenContext = {};

static LoadoutScreen *use_current_loadout_screen() {
    return static_cast<LoadoutScreen *>(use_context(&LoadoutScreenContext));
}

std::function<void()> use_push_loadout_screen() {
    ShooterGame *game = use_shooter_game();
    std::function<void()> request_quit = use_request_quit();
    client::ui::ScreenNavigator nav = client::ui::use_screen_navigator();
    return [game, request_quit, nav] {
        if (nav.push && game) nav.push(std::make_unique<LoadoutScreen>(game, request_quit));
    };
}

static bool use_compare_enabled() {
    LoadoutScreen *screen = use_current_loadout_screen();
    return screen ? screen->compare_enabled() : false;
}

static std::function<void(bool)> use_set_compare_enabled() {
    LoadoutScreen *screen = use_current_loadout_screen();
    client::ui::QueueUiWrite queue_write = client::ui::use_ui_write_queue();
    return [screen, queue_write](bool enabled) {
        if (screen && queue_write) {
            queue_write([screen, enabled] { screen->set_compare_enabled(enabled); });
        }
    };
}

static void LoadoutScreenView() {
    REACT_COMPONENT_BEGIN("LoadoutScreenView") {
        client::ui::ScreenNavigator nav = client::ui::use_screen_navigator();
        int weapon_count = use_shooter_weapon_count();
        int *selected_index       = use_state_int(use_selected_weapon_index());
        int *pending_action       = use_state_int(LOADOUT_ACTION_NONE);
        int *pending_weapon_index = use_state_int(*selected_index);
        int *pending_serial       = use_state_int(0);
        int *active_tab           = use_state_int(LOADOUT_TAB_WEAPONS);
        if (*selected_index < 0 || *selected_index >= weapon_count) {
            *selected_index = 0;
        }
        if (!weapon_in_tab(*selected_index, *active_tab)) {
            *selected_index = first_weapon_for_tab(*active_tab);
        }
        ShooterWeaponRead selected = use_weapon_read(*selected_index);
        if (!selected.valid) return;
        bool can_buy   = selected.can_buy;
        bool can_equip = selected.can_equip && !selected.equipped;
        std::function<void()> select_weapons_tab_weapon =
            use_select_weapon(first_weapon_for_tab(LOADOUT_TAB_WEAPONS));
        std::function<void()> select_gear_tab_weapon =
            use_select_weapon(first_weapon_for_tab(LOADOUT_TAB_GEAR));

        static char details[160];
        snprintf(details, sizeof(details), "%s: %s, cost %d, ammo %d",
                 selected.name, selected.role, selected.cost, selected.ammo);

        ::ui::ui_focus_push_scope({ .id = CLAY_ID("LoadoutScope"), .modal = true });
        ::ui::ui_focus_request_initial_focus(weapon_tile_id(*selected_index));

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
                CLAY_TEXT_CONFIG({ .textColor = { 236, 246, 242, 255 }, .fontSize = 26 }));
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
                    .on_select = [active_tab, selected_index, select_weapons_tab_weapon] {
                        if (active_tab) *active_tab = LOADOUT_TAB_WEAPONS;
                        if (selected_index) *selected_index = first_weapon_for_tab(LOADOUT_TAB_WEAPONS);
                        if (select_weapons_tab_weapon) select_weapons_tab_weapon();
                    },
                });
                ::ui::Selectable({
                    .id = CLAY_ID("GearTab"),
                    .label = "Gear",
                    .selected = *active_tab == LOADOUT_TAB_GEAR,
                    .on_select = [active_tab, selected_index, select_gear_tab_weapon] {
                        if (active_tab) *active_tab = LOADOUT_TAB_GEAR;
                        if (selected_index) *selected_index = first_weapon_for_tab(LOADOUT_TAB_GEAR);
                        if (select_gear_tab_weapon) select_gear_tab_weapon();
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
                                    .sizing = { CLAY_SIZING_FIT(0), CLAY_SIZING_FIT(0) },
                                    .childGap = 10,
                                    .layoutDirection = CLAY_LEFT_TO_RIGHT,
                                },
                            }) {
                                WeaponTile(row * 2, selected_index);
                                if (row == 0) {
                                    WeaponTile(row * 2 + 1, selected_index);
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
                            WeaponTile(3, selected_index);
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
                        CLAY_TEXT_CONFIG({ .textColor = { 226, 238, 236, 255 }, .fontSize = 14 }));
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
                        .on_confirm = [pending_action,
                                       pending_weapon_index,
                                       pending_serial,
                                       selected_index] {
                            if (pending_weapon_index && selected_index) {
                                *pending_weapon_index = *selected_index;
                            }
                            if (pending_serial) *pending_serial += 1;
                            if (pending_action) *pending_action = LOADOUT_ACTION_BUY;
                        },
                    });
                    ::ui::Button({
                        .id = CLAY_ID("EquipWeaponButton"),
                        .label = "Equip",
                        .disabled = !can_equip,
                        .on_confirm = [pending_action,
                                       pending_weapon_index,
                                       pending_serial,
                                       selected_index] {
                            if (pending_weapon_index && selected_index) {
                                *pending_weapon_index = *selected_index;
                            }
                            if (pending_serial) *pending_serial += 1;
                            if (pending_action) *pending_action = LOADOUT_ACTION_EQUIP;
                        },
                    });
                    ::ui::Button({
                        .id = CLAY_ID("BackFromLoadoutButton"),
                        .label = "Back",
                        .on_confirm = nav.pop_current,
                    });
                    CLAY_TEXT(::ui::clay_text("Equipment Slots"),
                        CLAY_TEXT_CONFIG({ .textColor = { 202, 218, 216, 255 }, .fontSize = 14 }));
                    EquipmentSlot(CLAY_ID("PrimarySlot"), "Primary", 0, selected_index);
                    EquipmentSlot(CLAY_ID("GearSlot"),    "Gear",    3, selected_index);
                }
            }
        }

        if (*pending_action != LOADOUT_ACTION_NONE) {
            LoadoutConfirmDialog(
                *pending_action, *pending_weapon_index, *pending_serial, pending_action);
        }

        ::ui::ui_focus_pop_scope();
    } REACT_COMPONENT_END();
}

void LoadoutScreen::build_ui() {
    ShooterProvider(game_, request_quit_, [this] {
        REACT_COMPONENT_BEGIN_KEY("LoadoutScreen", entry_id()) {
            PROVIDE(&LoadoutScreenContext, this) {
                LoadoutScreenView();
            }
        } REACT_COMPONENT_END();
    });
}

} // namespace shooter
