#include "equipment_slot.h"

#include <functional>
#include <stdint.h>

#include "../../../../../react.h"
#include "../../../../../ui/components/components.h"
#include "../../../hooks/shooter_weapons.h"
#include "../../../providers/shooter_provider.h"
#include "../loadout_state.h"

namespace shooter {

struct EquipmentSlotProps {
  const char *id = nullptr;
  const char *label = nullptr;
  int weapon_index = 0;
};

static ::ui::retained::UiElement
render_equipment_slot(const EquipmentSlotProps &props,
                      ::ui::retained::UiElementFrame &frame) {
  ShooterWeaponRead weapon = use_weapon_read(props.weapon_index);
  if (!weapon.valid)
    return frame.empty();

  int selected_index = use_selected_weapon_tile();
  std::function<void(int)> set_selected = use_set_selected_weapon_tile();
  std::function<void()> select = use_select_weapon(props.weapon_index);
  bool selected = selected_index == props.weapon_index;
  namespace retained = ::ui::retained;
  namespace components = ::ui::components;

  const char *slot_text = use_text_storage(
      "%s: %s", props.label, weapon.owned ? weapon.name : "empty");

  return components::Selectable(
      frame,
      {
          .key = props.id,
          .id = props.id,
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
          .children = frame.children({
              components::Text(frame,
                               {
                                   .key = "label",
                                   .value = slot_text,
                                   .height = retained::Length::points(16.0f),
                                   .text_color = {226, 238, 236, 255},
                                   .font_size = 14,
                               }),
          }),
          .on_focus =
              [set_selected, weapon_index = props.weapon_index] {
                if (set_selected)
                  set_selected(weapon_index);
              },
          .on_confirm =
              [set_selected, weapon_index = props.weapon_index, select] {
                if (set_selected)
                  set_selected(weapon_index);
                if (select)
                  select();
              },
      });
}

::ui::retained::UiElement EquipmentSlot(::ui::retained::UiElementFrame &frame,
                                        const char *id, const char *label,
                                        int weapon_index) {
  return frame.component("EquipmentSlot",
                         EquipmentSlotProps{
                             .id = id,
                             .label = label,
                             .weapon_index = weapon_index,
                         },
                         render_equipment_slot, id);
}

} // namespace shooter
