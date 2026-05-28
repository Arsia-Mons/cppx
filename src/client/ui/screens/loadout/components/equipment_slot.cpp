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

static ::ui::UiElement render_equipment_slot(const EquipmentSlotProps &props) {
  ShooterWeaponRead weapon = use_weapon_read(props.weapon_index);
  if (!weapon.valid)
    return ::ui::empty();

  int selected_index = use_selected_weapon_tile();
  std::function<void(int)> set_selected = use_set_selected_weapon_tile();
  std::function<void()> select = use_select_weapon(props.weapon_index);
  bool selected = selected_index == props.weapon_index;
  namespace components = ::ui::components;

  const char *slot_text = use_text_storage(
      "%s: %s", props.label, weapon.owned ? weapon.name : "empty");

  return components::Button({
      .key = props.id,
      .id = props.id,
      .autofocus = selected,
      .accessibility =
          {
              .label = slot_text,
          },
      .on_focus =
          [set_selected,
           weapon_index = props.weapon_index](const ::ui::FocusEvent &) {
            if (set_selected)
              set_selected(weapon_index);
          },
      .label = slot_text,
      .children = ::ui::children({
          components::Text({
              .key = "label",
              .value = slot_text,
              .style =
                  {
                      .height = ::ui::Length::points(16.0f),
                      .text = {226, 238, 236, 255},
                      .font_size = 14,
                  },
          }),
      }),
      .on_activate =
          [set_selected, weapon_index = props.weapon_index,
           select](const ::ui::ActivationEvent &) {
            if (set_selected)
              set_selected(weapon_index);
            if (select)
              select();
          },
      .style =
          {
              .width = ::ui::Length::points(232.0f),
              .height = ::ui::Length::points(38.0f),
              .align_items = ::ui::AlignItems::Start,
              .justify_content = ::ui::JustifyContent::Center,
              .padding = {10.0f, 10.0f, 8.0f, 8.0f},
              .background = selected ? ::ui::Color{35, 72, 62, 255}
                                     : ::ui::Color{24, 28, 36, 255},
              .border = selected ? ::ui::Color{122, 176, 238, 255}
                                 : ::ui::Color{78, 88, 104, 255},
              .border_width = selected ? 2.0f : 1.0f,
          },
  });
}

::ui::UiElement EquipmentSlot(const char *id, const char *label,
                              int weapon_index) {
  return ::ui::component("EquipmentSlot",
                         EquipmentSlotProps{
                             .id = id,
                             .label = label,
                             .weapon_index = weapon_index,
                         },
                         render_equipment_slot, id);
}

} // namespace shooter
