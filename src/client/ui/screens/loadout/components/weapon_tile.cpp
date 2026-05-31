#include "weapon_tile.h"

#include <functional>
#include <stdint.h>

#include "../../../../../react.h"
#include "../../../../../ui/components/components.h"
#include "client/ui/components/tokens.h"
#include "client/ui/screens/loadout/hooks/use_loadout.h"
#include "client/ui/screens/loadout/hooks/use_weapons.h"

namespace shooter {

namespace {

} // namespace

const char *weapon_tile_key(int index) {
  switch (index) {
  case 0:
    return "weapon-0";
  case 1:
    return "weapon-1";
  case 2:
    return "weapon-2";
  case 3:
    return "weapon-3";
  default:
    return "weapon";
  }
}

::ui::UiElement WeaponTile(const WeaponTileProps &props) {
  int index = props.index;
  WeaponsValue weapons = use_weapons();
  WeaponsValue::Weapon weapon = weapons.get_weapon(index);
  if (!weapon.valid)
    return ::ui::empty();

  LoadoutValue loadout = use_loadout();
  int selected_index = loadout.selected_weapon_tile;
  std::function<void(int)> set_selected = loadout.set_selected_weapon_tile;
  bool selected = selected_index == index;
  bool disabled = weapon.disabled;
  namespace components = ::ui::components;

  const char *detail =
      use_text_storage("%s  DMG %d  %s", weapon.role, weapon.damage,
                       weapon.owned ? (weapon.equipped ? "equipped" : "owned")
                                    : (disabled ? "locked" : "available"));

  return ::ui::component(
      "Button",
      components::ButtonProps{
          .key = props.key ? props.key : weapon_tile_key(index),
          .id = WEAPON_TILE_CONTROL_ID,
          .id_offset = index,
          .disabled = disabled,
          .autofocus = selected,
          .accessibility =
              {
                  .label = weapon.name,
              },
          .on_focus =
              [set_selected, index](const ::ui::FocusEvent &) {
                if (set_selected)
                  set_selected(index);
              },
          .on_activate =
              [set_selected, index,
               weapons](const ::ui::ActivationEvent &) {
                if (set_selected)
                  set_selected(index);
                weapons.select(index);
              },
          .children =
              ::ui::children({
                  ::ui::component(
                      "Text",
                      components::TextProps{
                          .key = "name",
                          .value = weapon.name,
                          .layout =
                              {
                                  .height = ::ui::Length::points(18.0f),
                              },
                          .style = tokens::text_patch({238, 246, 244, 255}, 16),
                      },
                      components::Text),
                  ::ui::component(
                      "Text",
                      components::TextProps{
                          .key = "detail",
                          .value = detail,
                          .layout =
                              {
                                  .height = ::ui::Length::points(16.0f),
                              },
                          .style = tokens::text_patch(
                              disabled ? ::ui::Color{142, 148, 150, 255}
                                       : ::ui::Color{184, 204, 204, 255},
                              12),
                      },
                      components::Text),
              }),
          // Button paint comes from the theme button role (resolved as .visual
          // inside Button); selected/disabled states are conveyed by focus and
          // the disabled interaction. Only layout fields live in .layout.
          .layout =
              {
                  .align_items = ::ui::AlignItems::Start,
                  .justify_content = ::ui::JustifyContent::Start,
                  .width = ::ui::Length::points(190.0f),
                  .height = ::ui::Length::points(78.0f),
                  .padding = {10.0f, 10.0f, 10.0f, 10.0f},
                  .gap = 5.0f,
                  .border_width = selected ? 2.0f : 1.0f,
              },
      },
      components::Button);
}

bool weapon_in_tab(int index, int tab) {
  if (tab == LOADOUT_TAB_GEAR)
    return index == 3;
  return index >= 0 && index < 3;
}

int first_weapon_for_tab(int tab) { return tab == LOADOUT_TAB_GEAR ? 3 : 0; }

} // namespace shooter
