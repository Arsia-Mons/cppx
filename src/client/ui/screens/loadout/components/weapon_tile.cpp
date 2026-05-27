#include "weapon_tile.h"

#include <functional>
#include <stdint.h>

#include "../loadout_state.h"
#include "../../../hooks/shooter_weapons.h"
#include "../../../providers/shooter_provider.h"
#include "../../../../../react.h"
#include "../../../../../ui/primitives/clay_text.h"
#include "../../../../../ui/primitives/focusable.h"
#include "../../../../../ui/primitives/visual_state.h"

namespace shooter {

Clay_ElementId weapon_tile_id(int index) {
    return CLAY_IDI("WeaponTile", index);
}

bool weapon_in_tab(int index, int tab) {
    if (tab == LOADOUT_TAB_GEAR) return index == 3;
    return index >= 0 && index < 3;
}

int first_weapon_for_tab(int tab) {
    return tab == LOADOUT_TAB_GEAR ? 3 : 0;
}

void WeaponTile(int index) {
    REACT_FRAGMENT_COMPONENT_BEGIN_KEY("WeaponTile", (uint32_t)index) {
        ShooterWeaponRead weapon = use_weapon_read(index);
        if (weapon.valid) {
            int selected_index = use_selected_weapon_tile();
            std::function<void(int)> set_selected = use_set_selected_weapon_tile();
            std::function<void()>    select       = use_select_weapon(index);
            bool selected = selected_index == index;
            bool disabled = weapon.disabled;

            const char *detail = use_text_storage("%s  DMG %d  %s",
                weapon.role,
                weapon.damage,
                weapon.owned ? (weapon.equipped ? "equipped" : "owned")
                             : (disabled ? "locked" : "available"));

            ::ui::Focusable({
                .id = weapon_tile_id(index),
                .disabled = disabled,
                .on_confirm = [set_selected, index, select] {
                    if (set_selected) set_selected(index);
                    if (select) select();
                },
                // on_focus updates UI selection only — it does NOT call
                // select() against game state (that's the dialog's job).
                .on_focus = [set_selected, index] {
                    if (set_selected) set_selected(index);
                },
            }, [&](const ::ui::UiFocusableState &focus) {
                ::ui::VisualState visual = ::ui::derive_visual_state(focus, {
                    .selected = selected,
                    .disabled = disabled,
                });
                uint16_t border_width = visual.targeted ? 2 : 1;
                CLAY({
                    .id = focus.id,
                    .layout = {
                        .sizing = { CLAY_SIZING_FIXED(190), CLAY_SIZING_FIXED(78) },
                        .padding = CLAY_PADDING_ALL(10),
                        .childGap = 5,
                        .layoutDirection = CLAY_TOP_TO_BOTTOM,
                    },
                    .backgroundColor = visual.chosen
                        ? Clay_Color{ 35, 72, 62, 255 }
                        : Clay_Color{ 24, 31, 36, 255 },
                    .cornerRadius = CLAY_CORNER_RADIUS(4),
                    .border = {
                        .color = disabled
                            ? Clay_Color{ 58, 62, 66, 255 }
                            : Clay_Color{ 102, 142, 150, 255 },
                        .width = CLAY_BORDER_OUTSIDE(border_width),
                    },
                }) {
                    CLAY_TEXT(::ui::clay_text(weapon.name),
                        CLAY_TEXT_CONFIG({ .textColor = { 238, 246, 244, 255 }, .fontSize = 16 }));
                    CLAY_TEXT(::ui::clay_text(detail),
                        CLAY_TEXT_CONFIG({
                            .textColor = disabled
                                ? Clay_Color{ 142, 148, 150, 255 }
                                : Clay_Color{ 184, 204, 204, 255 },
                            .fontSize = 12,
                        }));
                }
            });
        }
    } REACT_FRAGMENT_COMPONENT_END();
}

} // namespace shooter
