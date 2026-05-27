#include "equipment_slot.h"

#include <functional>
#include <stdint.h>

#include "../loadout_state.h"
#include "../../../hooks/shooter_weapons.h"
#include "../../../providers/shooter_provider.h"
#include "../../../../../react.h"
#include "../../../../../ui/retained/components.h"

namespace shooter {

void EquipmentSlot(const char    *id,
                   const char    *label,
                   int            weapon_index) {
    REACT_RETAINED_COMPONENT_BEGIN_KEY("EquipmentSlot", (uint32_t)weapon_index) {
        ShooterWeaponRead weapon = use_weapon_read(weapon_index);
        if (weapon.valid) {
            int selected_index = use_selected_weapon_tile();
            std::function<void(int)> set_selected = use_set_selected_weapon_tile();
            std::function<void()>    select       = use_select_weapon(weapon_index);
            bool selected = selected_index == weapon_index;
            namespace retained = ::ui::retained;

            const char *slot_text = use_text_storage("%s: %s",
                label,
                weapon.owned ? weapon.name : "empty");

            retained::Selectable({
                .key = id,
                .id = id,
                .label = slot_text,
                .selected = selected,
                .initial_focus = selected,
                .width = retained::Length::points(232.0f),
                .height = retained::Length::points(38.0f),
                .align_items = retained::AlignItems::Start,
                .justify_content = retained::JustifyContent::Center,
                .padding = {10.0f, 10.0f, 8.0f, 8.0f},
                .background = selected ? retained::Color{35, 72, 62, 255}
                                       : retained::Color{24, 28, 36, 255},
                .border = selected ? retained::Color{122, 176, 238, 255}
                                   : retained::Color{78, 88, 104, 255},
                .border_width = selected ? 2.0f : 1.0f,
                .on_confirm = [set_selected, weapon_index, select] {
                    if (set_selected) set_selected(weapon_index);
                    if (select) select();
                },
                // on_focus updates UI selection only — it does NOT call
                // select() against game state.
                .on_focus = [set_selected, weapon_index] {
                    if (set_selected) set_selected(weapon_index);
                },
            }, [&] {
                retained::Text({
                    .key = "label",
                    .value = slot_text,
                    .height = retained::Length::points(16.0f),
                    .text_color = {226, 238, 236, 255},
                    .font_size = 14,
                });
            });
        }
    } REACT_RETAINED_COMPONENT_END();
}

} // namespace shooter
