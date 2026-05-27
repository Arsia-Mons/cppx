#include "confirm_dialog.h"

#include <functional>
#include <stdint.h>

#include <clay.h>

#include "../loadout_state.h"
#include "../../../hooks/shooter_weapons.h"
#include "../../../providers/shooter_provider.h"
#include "../../../../../react.h"
#include "../../../../../ui/focus/ui_focus.h"
#include "../../../../../ui/primitives/button.h"
#include "../../../../../ui/primitives/clay_text.h"

namespace shooter {

void LoadoutConfirmDialog() {
    LoadoutPendingAction pending = use_pending_loadout_action();
    if (pending.action == LOADOUT_ACTION_NONE) return;

    REACT_COMPONENT_BEGIN_KEY("LoadoutConfirmDialog", pending.generation) {
        ShooterWeaponRead weapon = use_weapon_read(pending.weapon_index);
        std::function<void()> buy   = use_buy_weapon(pending.weapon_index);
        std::function<void()> equip = use_equip_weapon(pending.weapon_index);
        std::function<void()> close = use_clear_pending_loadout_action();
        if (weapon.valid) {
            const char *title = use_text_storage("%s",
                pending.action == LOADOUT_ACTION_BUY ? "Confirm Buy" : "Confirm Equip");
            const char *message = pending.action == LOADOUT_ACTION_BUY
                ? use_text_storage("Buy %s for %d credits?", weapon.name, weapon.cost)
                : use_text_storage("Equip %s as active weapon?", weapon.name);

            ::ui::ui_focus_push_scope({
                .id = CLAY_IDI("LoadoutConfirmScope", (int32_t)pending.generation),
                .modal = true,
                .wrap = true,
            });
            ::ui::ui_focus_request_initial_focus(CLAY_ID("ConfirmLoadoutActionButton"));

            int action = pending.action;
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
                                if (close) close();
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
