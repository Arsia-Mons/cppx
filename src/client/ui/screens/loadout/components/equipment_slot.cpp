#include "equipment_slot.h"

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

void EquipmentSlot(Clay_ElementId id,
                   const char    *label,
                   int            weapon_index) {
    REACT_FRAGMENT_COMPONENT_BEGIN_KEY("EquipmentSlot", id.id) {
        ShooterWeaponRead weapon = use_weapon_read(weapon_index);
        if (weapon.valid) {
            int selected_index = use_selected_weapon_tile();
            std::function<void(int)> set_selected = use_set_selected_weapon_tile();
            std::function<void()>    select       = use_select_weapon(weapon_index);
            bool selected = selected_index == weapon_index;

            const char *slot_text = use_text_storage("%s: %s",
                label,
                weapon.owned ? weapon.name : "empty");

            ::ui::Focusable({
                .id = id,
                .on_confirm = [set_selected, weapon_index, select] {
                    if (set_selected) set_selected(weapon_index);
                    if (select) select();
                },
                // on_focus updates UI selection only — it does NOT call
                // select() against game state.
                .on_focus = [set_selected, weapon_index] {
                    if (set_selected) set_selected(weapon_index);
                },
            }, [&](const ::ui::UiFocusableState &focus) {
                ::ui::VisualState visual = ::ui::derive_visual_state(focus, {
                    .selected = selected,
                });
                uint16_t border_width = visual.targeted ? 2 : 1;
                CLAY({
                    .id = focus.id,
                    .layout = {
                        .sizing = { CLAY_SIZING_FIXED(232), CLAY_SIZING_FIXED(38) },
                        .padding = { 10, 10, 8, 8 },
                        .childAlignment = { CLAY_ALIGN_X_LEFT, CLAY_ALIGN_Y_CENTER },
                    },
                    .backgroundColor = visual.chosen
                        ? Clay_Color{ 35, 72, 62, 255 }
                        : Clay_Color{ 24, 28, 36, 255 },
                    .cornerRadius = CLAY_CORNER_RADIUS(4),
                    .border = {
                        .color = visual.targeted
                            ? Clay_Color{ 122, 176, 238, 255 }
                            : Clay_Color{ 78, 88, 104, 255 },
                        .width = CLAY_BORDER_OUTSIDE(border_width),
                    },
                }) {
                    CLAY_TEXT(::ui::clay_text(slot_text),
                        CLAY_TEXT_CONFIG({ .textColor = { 226, 238, 236, 255 }, .fontSize = 14 }));
                }
            });
        }
    } REACT_FRAGMENT_COMPONENT_END();
}

} // namespace shooter
