#include "confirm_dialog.h"

#include <functional>
#include <stdint.h>

#include "../loadout_state.h"
#include "../../../hooks/shooter_weapons.h"
#include "../../../providers/shooter_provider.h"
#include "../../../../../react.h"
#include "../../../../../ui/retained/components.h"

namespace shooter {

static void LoadoutConfirmDialogBody(LoadoutPendingAction pending) {
    REACT_RETAINED_COMPONENT_BEGIN_KEY("LoadoutConfirmDialogBody",
                                       pending.generation) {
        ShooterWeaponRead weapon = use_weapon_read(pending.weapon_index);
        std::function<void()> buy   = use_buy_weapon(pending.weapon_index);
        std::function<void()> equip = use_equip_weapon(pending.weapon_index);
        std::function<void()> close = use_clear_pending_loadout_action();
        if (weapon.valid) {
            namespace retained = ::ui::retained;
            const char *title = use_text_storage(
                "%s",
                pending.action == LOADOUT_ACTION_BUY ? "Confirm Buy"
                                                     : "Confirm Equip");
            const char *message =
                pending.action == LOADOUT_ACTION_BUY
                    ? use_text_storage("Buy %s for %d credits?", weapon.name,
                                       weapon.cost)
                    : use_text_storage("Equip %s as active weapon?",
                                       weapon.name);

            int action = pending.action;
            retained::Panel(
                {
                    .key = "scrim",
                    .width = retained::Length::percent(100.0f),
                    .height = retained::Length::percent(100.0f),
                    .align_items = retained::AlignItems::Center,
                    .justify_content = retained::JustifyContent::Center,
                    .modal = true,
                    .background = {0, 0, 0, 160},
                },
                [&] {
                    retained::Panel(
                        {
                            .key = "panel",
                            .width = retained::Length::points(360.0f),
                            .padding = {18.0f, 18.0f, 18.0f, 18.0f},
                            .gap = 12.0f,
                            .background = {18, 26, 32, 255},
                            .border = {92, 116, 126, 255},
                            .border_width = 1.0f,
                        },
                        [&] {
                            retained::Text({
                                .key = "title",
                                .value = title,
                                .height = retained::Length::points(24.0f),
                                .text_color = {240, 248, 244, 255},
                                .font_size = 22,
                            });
                            retained::Text({
                                .key = "message",
                                .value = message,
                                .height = retained::Length::points(18.0f),
                                .text_color = {202, 218, 216, 255},
                                .font_size = 15,
                            });
                            retained::Panel(
                                {
                                    .key = "actions",
                                    .direction = retained::FlexDirection::Row,
                                    .gap = 10.0f,
                                },
                                [&] {
                                    retained::Button({
                                        .key = "confirm",
                                        .id = "ConfirmLoadoutActionButton",
                                        .label = "Confirm",
                                        .initial_focus = true,
                                        .on_confirm =
                                            [action, buy, equip, close] {
                                                if (action == LOADOUT_ACTION_BUY) {
                                                    if (buy) buy();
                                                } else if (action ==
                                                           LOADOUT_ACTION_EQUIP) {
                                                    if (equip) equip();
                                                }
                                                if (close) close();
                                            },
                                    });
                                    retained::Button({
                                        .key = "cancel",
                                        .id = "CancelLoadoutActionButton",
                                        .label = "Cancel",
                                        .on_confirm = close,
                                    });
                                });
                        });
                });
        }
    } REACT_RETAINED_COMPONENT_END();
}

void LoadoutConfirmDialog() {
    REACT_RETAINED_COMPONENT_BEGIN("LoadoutConfirmDialog") {
        LoadoutPendingAction pending = use_pending_loadout_action();
        if (pending.action != LOADOUT_ACTION_NONE) {
            LoadoutConfirmDialogBody(pending);
        }
    } REACT_RETAINED_COMPONENT_END();
}

} // namespace shooter
