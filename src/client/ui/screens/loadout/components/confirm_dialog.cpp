#include "confirm_dialog.h"

#include <functional>
#include <stdio.h>
#include <stdint.h>

#include <clay.h>

#include "../../../hooks/shooter_weapons.h"
#include "../../../providers/shooter_provider.h"
#include "../../../../../react.h"
#include "../../../../../ui/focus/ui_focus.h"
#include "../../../../../ui/primitives/button.h"
#include "../../../../../ui/primitives/clay_text.h"

namespace shooter {

void LoadoutConfirmDialog(int  action,
                          int  weapon_index,
                          int  serial,
                          int *pending_action) {
    REACT_COMPONENT_BEGIN_KEY("LoadoutConfirmDialog", (uint32_t)((serial << 8) | action)) {
        ShooterWeaponRead weapon = use_weapon_read(weapon_index);
        std::function<void()> buy   = use_buy_weapon(weapon_index);
        std::function<void()> equip = use_equip_weapon(weapon_index);
        bool valid = weapon.valid && action != LOADOUT_ACTION_NONE;
        if (valid) {
            static char title[64];
            static char message[128];
            if (action == LOADOUT_ACTION_BUY) {
                snprintf(title, sizeof(title), "Confirm Buy");
                snprintf(message, sizeof(message), "Buy %s for %d credits?",
                         weapon.name, weapon.cost);
            } else {
                snprintf(title, sizeof(title), "Confirm Equip");
                snprintf(message, sizeof(message), "Equip %s as active weapon?",
                         weapon.name);
            }

            auto close = [pending_action] {
                if (pending_action) *pending_action = LOADOUT_ACTION_NONE;
            };

            ::ui::ui_focus_push_scope({
                .id = CLAY_IDI("LoadoutConfirmScope", serial),
                .modal = true,
                .wrap = true,
            });
            ::ui::ui_focus_request_initial_focus(CLAY_ID("ConfirmLoadoutActionButton"));

            CLAY({
                .id = CLAY_ID("LoadoutConfirmScrim"),
                .layout = {
                    .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) },
                    .childAlignment = { CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER },
                },
                .backgroundColor = { 0, 0, 0, 160 },
                .floating = {
                    .zIndex = 300,
                    .pointerCaptureMode = CLAY_POINTER_CAPTURE_MODE_CAPTURE,
                    .attachTo = CLAY_ATTACH_TO_ROOT,
                },
            }) {
                CLAY({
                    .id = CLAY_ID("LoadoutConfirmPanel"),
                    .layout = {
                        .sizing = { CLAY_SIZING_FIXED(360), CLAY_SIZING_FIT(0) },
                        .padding = CLAY_PADDING_ALL(18),
                        .childGap = 12,
                        .layoutDirection = CLAY_TOP_TO_BOTTOM,
                    },
                    .backgroundColor = { 18, 26, 32, 255 },
                    .cornerRadius = CLAY_CORNER_RADIUS(4),
                    .border = {
                        .color = { 92, 116, 126, 255 },
                        .width = CLAY_BORDER_OUTSIDE(1),
                    },
                }) {
                    CLAY_TEXT(::ui::clay_text(title),
                        CLAY_TEXT_CONFIG({ .textColor = { 240, 248, 244, 255 }, .fontSize = 22 }));
                    CLAY_TEXT(::ui::clay_text(message),
                        CLAY_TEXT_CONFIG({ .textColor = { 202, 218, 216, 255 }, .fontSize = 15 }));
                    CLAY({
                        .id = CLAY_ID("LoadoutConfirmActions"),
                        .layout = {
                            .sizing = { CLAY_SIZING_FIT(0), CLAY_SIZING_FIT(0) },
                            .childGap = 10,
                            .layoutDirection = CLAY_LEFT_TO_RIGHT,
                        },
                    }) {
                        ::ui::Button({
                            .id = CLAY_ID("ConfirmLoadoutActionButton"),
                            .label = "Confirm",
                            .on_confirm = [action, buy, equip, close] {
                                if (action == LOADOUT_ACTION_BUY) {
                                    if (buy) buy();
                                } else if (action == LOADOUT_ACTION_EQUIP) {
                                    if (equip) equip();
                                }
                                close();
                            },
                        });
                        ::ui::Button({
                            .id = CLAY_ID("CancelLoadoutActionButton"),
                            .label = "Cancel",
                            .on_confirm = close,
                        });
                    }
                }
            }

            ::ui::ui_focus_pop_scope();
        }
    } REACT_COMPONENT_END();
}

} // namespace shooter
