#include "weapon_tile.h"

#include <functional>
#include <stdint.h>

#include "../../../../../react.h"
#include "../../../../../ui/components/components.h"
#include "../../../hooks/shooter_weapons.h"
#include "../../../providers/shooter_provider.h"
#include "../loadout_state.h"

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
  ShooterWeaponRead weapon = use_weapon_read(index);
  if (!weapon.valid)
    return ::ui::empty();

  int selected_index = use_selected_weapon_tile();
  std::function<void(int)> set_selected = use_set_selected_weapon_tile();
  std::function<void()> select = use_select_weapon(index);
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
              [set_selected, index, select](const ::ui::ActivationEvent &) {
                if (set_selected)
                  set_selected(index);
                if (select)
                  select();
              },
          .style =
              {
                  .align_items = ::ui::AlignItems::Start,
                  .justify_content = ::ui::JustifyContent::Start,
                  .width = ::ui::Length::points(190.0f),
                  .height = ::ui::Length::points(78.0f),
                  .padding = {10.0f, 10.0f, 10.0f, 10.0f},
                  .gap = 5.0f,
                  .background = selected ? ::ui::Color{35, 72, 62, 255}
                                         : ::ui::Color{24, 31, 36, 255},
                  .border = disabled ? ::ui::Color{58, 62, 66, 255}
                                     : ::ui::Color{102, 142, 150, 255},
                  .border_width = selected ? 2.0f : 1.0f,
              },
          .children =
              ::ui::children({
                  ::ui::component(
                      "Text",
                      components::TextProps{
                          .key = "name",
                          .value = weapon.name,
                          .style =
                              {
                                  .height = ::ui::Length::points(18.0f),
                                  .text = {238, 246, 244, 255},
                                  .font_size = 16,
                              },
                      },
                      components::Text),
                  ::ui::component(
                      "Text",
                      components::TextProps{
                          .key = "detail",
                          .value = detail,
                          .style =
                              {
                                  .height = ::ui::Length::points(16.0f),
                                  .text = disabled ? ::ui::Color{142, 148,
                                                                 150, 255}
                                                   : ::ui::Color{184, 204,
                                                                 204, 255},
                                  .font_size = 12,
                              },
                      },
                      components::Text),
              }),
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
