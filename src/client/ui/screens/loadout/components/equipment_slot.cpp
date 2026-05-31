#include "equipment_slot.h"

#include <functional>
#include <stdint.h>

#include "../../../../../react.h"
#include "../../../../../ui/components/components.h"
#include "client/ui/components/tokens.h"
#include "client/ui/screens/loadout/hooks/use_loadout.h"
#include "client/ui/screens/loadout/hooks/use_weapons.h"

namespace shooter {

::ui::UiElement EquipmentSlot(const EquipmentSlotProps &props) {
  WeaponsValue weapons = use_weapons();
  WeaponsValue::Weapon weapon = weapons.get_weapon(props.weapon_index);
  if (!weapon.valid)
    return ::ui::empty();

  LoadoutValue loadout = use_loadout();
  int selected_index = loadout.selected_weapon_tile;
  std::function<void(int)> set_selected = loadout.set_selected_weapon_tile;
  bool selected = selected_index == props.weapon_index;
  namespace components = ::ui::components;

  const char *slot_text = use_text_storage(
      "%s: %s", props.label, weapon.owned ? weapon.name : "empty");

  return ::ui::component(
      "Button",
      components::ButtonProps{
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
          .on_activate =
              [set_selected, weapon_index = props.weapon_index,
               weapons](const ::ui::ActivationEvent &) {
                if (set_selected)
                  set_selected(weapon_index);
                weapons.select(weapon_index);
              },
          .children = ::ui::children({
              ::ui::component(
                  "Text",
                  components::TextProps{
                      .key = "label",
                      .value = slot_text,
                      .layout =
                          {
                              .height = ::ui::Length::points(16.0f),
                          },
                      .style = tokens::text_patch({226, 238, 236, 255}, 14),
                  },
                  components::Text),
          }),
          // Button paint comes from the theme button role (resolved as .visual
          // inside Button); the selected state is conveyed by focus (autofocus=
          // selected -> focus ring). Only layout fields live in .layout.
          .layout =
              {
                  .align_items = ::ui::AlignItems::Start,
                  .justify_content = ::ui::JustifyContent::Center,
                  .width = ::ui::Length::points(232.0f),
                  .height = ::ui::Length::points(38.0f),
                  .padding = {10.0f, 10.0f, 8.0f, 8.0f},
                  .border_width = selected ? 2.0f : 1.0f,
              },
      },
      components::Button);
}

} // namespace shooter
