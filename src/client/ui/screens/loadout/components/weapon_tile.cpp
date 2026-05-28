#include "weapon_tile.h"

#include <functional>
#include <stdint.h>

#include "../../../../../react.h"
#include "../../../../../ui/retained/element_components.h"
#include "../../../hooks/shooter_weapons.h"
#include "../../../providers/shooter_provider.h"
#include "../loadout_state.h"

namespace shooter {

namespace {

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

struct WeaponTileProps {
  int index = 0;
};

::ui::retained::UiElement
render_weapon_tile(const WeaponTileProps &props,
                   ::ui::retained::UiElementFrame &frame) {
  int index = props.index;
  ShooterWeaponRead weapon = use_weapon_read(index);
  if (!weapon.valid)
    return frame.empty();

  int selected_index = use_selected_weapon_tile();
  std::function<void(int)> set_selected = use_set_selected_weapon_tile();
  std::function<void()> select = use_select_weapon(index);
  bool selected = selected_index == index;
  bool disabled = weapon.disabled;
  namespace retained = ::ui::retained;

  const char *detail =
      use_text_storage("%s  DMG %d  %s", weapon.role, weapon.damage,
                       weapon.owned ? (weapon.equipped ? "equipped" : "owned")
                                    : (disabled ? "locked" : "available"));

  return retained::SelectableElement(
      frame,
      {
          .key = weapon_tile_key(index),
          .id = WEAPON_TILE_CONTROL_ID,
          .offset = index,
          .selected = selected,
          .disabled = disabled,
          .initial_focus = selected,
          .width = retained::Length::points(190.0f),
          .height = retained::Length::points(78.0f),
          .align_items = retained::AlignItems::Start,
          .justify_content = retained::JustifyContent::Start,
          .padding = {10.0f, 10.0f, 10.0f, 10.0f},
          .gap = 5.0f,
          .background = selected ? retained::Color{35, 72, 62, 255}
                                 : retained::Color{24, 31, 36, 255},
          .border = disabled ? retained::Color{58, 62, 66, 255}
                             : retained::Color{102, 142, 150, 255},
          .border_width = selected ? 2.0f : 1.0f,
          .children = frame.children({
              retained::TextElement(
                  frame,
                  {
                      .key = "name",
                      .value = weapon.name,
                      .height = retained::Length::points(18.0f),
                      .text_color = {238, 246, 244, 255},
                      .font_size = 16,
                  }),
              retained::TextElement(
                  frame,
                  {
                      .key = "detail",
                      .value = detail,
                      .height = retained::Length::points(16.0f),
                      .text_color = disabled
                                        ? retained::Color{142, 148, 150, 255}
                                        : retained::Color{184, 204, 204, 255},
                      .font_size = 12,
                  }),
          }),
          .on_focus =
              [set_selected, index] {
                if (set_selected)
                  set_selected(index);
              },
          .on_confirm =
              [set_selected, index, select] {
                if (set_selected)
                  set_selected(index);
                if (select)
                  select();
              },
      });
}

} // namespace

bool weapon_in_tab(int index, int tab) {
  if (tab == LOADOUT_TAB_GEAR)
    return index == 3;
  return index >= 0 && index < 3;
}

int first_weapon_for_tab(int tab) { return tab == LOADOUT_TAB_GEAR ? 3 : 0; }

::ui::retained::UiElement WeaponTile(::ui::retained::UiElementFrame &frame,
                                     int index) {
  return frame.component("WeaponTile", WeaponTileProps{.index = index},
                         render_weapon_tile, weapon_tile_key(index));
}

} // namespace shooter
