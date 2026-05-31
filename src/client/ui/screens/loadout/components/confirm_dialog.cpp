#include "confirm_dialog.h"

#include <functional>
#include <stdint.h>
#include <stdio.h>

#include "../../../../../react.h"
#include "../../../../../ui/components/components.h"
#include "client/ui/components/tokens.h"
#include "client/ui/screens/loadout/hooks/use_loadout.h"
#include "client/ui/screens/loadout/hooks/use_weapons.h"

namespace shooter {

namespace {

const char *confirm_body_key(uint32_t generation) {
  char key[64] = {};
  snprintf(key, sizeof(key), "confirm-%u", generation);
  return ::ui::copy_string(key);
}

::ui::UiElement
LoadoutConfirmDialogBody(const LoadoutValue::PendingAction &pending) {
  WeaponsValue weapons = use_weapons();
  WeaponsValue::Weapon weapon = weapons.get_weapon(pending.weapon_index);
  LoadoutValue loadout = use_loadout();
  std::function<void()> close = loadout.clear_pending_action;
  if (!weapon.valid)
    return ::ui::empty();

  namespace components = ::ui::components;
  const char *title = use_text_storage(
      "%s",
      pending.action == LoadoutValue::Action::Buy ? "Confirm Buy"
                                                  : "Confirm Equip");
  const char *message =
      pending.action == LoadoutValue::Action::Buy
          ? use_text_storage("Buy %s for %d credits?", weapon.name, weapon.cost)
          : use_text_storage("Equip %s as active weapon?", weapon.name);
  LoadoutValue::Action action = pending.action;

  return ::ui::component(
      "Dialog",
      components::DialogProps{
          .key = "scrim",
          .layout =
              {
                  .align_items = ::ui::AlignItems::Center,
                  .justify_content = ::ui::JustifyContent::Center,
                  .width = ::ui::Length::percent(100.0f),
                  .height = ::ui::Length::percent(100.0f),
              },
          .style = tokens::fill_patch({0, 0, 0, 160}),
          .children =
              ::ui::children(
                  {
                      ::ui::component(
                          "Box",
                          components::BoxProps{
                              .key = "panel",
                              .layout =
                                  {
                                      .width = ::ui::Length::points(360.0f),
                                      .padding = {18.0f, 18.0f, 18.0f, 18.0f},
                                      .gap = 12.0f,
                                      .border_width = 1.0f,
                                  },
                              .style = tokens::panel_patch({18, 26, 32, 255},
                                                            {92, 116, 126, 255}),
                              .children = ::ui::children(
                                  {
                                      ::ui::component(
                                          "Text",
                                          components::TextProps{
                                              .key = "title",
                                              .value = title,
                                              .layout =
                                                  {
                                                      .height =
                                                          ::ui::Length::points(
                                                              24.0f),
                                                  },
                                              .style = tokens::text_patch(
                                                  {240, 248, 244, 255}, 22),
                                          },
                                          components::Text),
                                      ::ui::component(
                                          "Text",
                                          components::TextProps{
                                              .key = "message",
                                              .value = message,
                                              .layout =
                                                  {
                                                      .height =
                                                          ::ui::Length::points(
                                                              18.0f),
                                                  },
                                              .style = tokens::text_patch(
                                                  {202, 218, 216, 255}, 15),
                                          },
                                          components::Text),
                                      ::ui::component(
                                          "Box",
                                          components::BoxProps{
                                              .key = "actions",
                                              .layout =
                                                  {
                                                      .direction =
                                                          ::ui::FlexDirection::
                                                              Row,
                                                      .gap = 10.0f,
                                                  },
                                              .children = ::ui::children({
                                                  ::ui::component(
                                                      "Button",
                                                      components::ButtonProps{
                                                          .key = "confirm",
                                                          .id = "ConfirmLoadoutActi"
                                                                "onButton",
                                                          .autofocus = true,
                                                          .label = "Confirm",
                                                          .on_activate =
                                                              [action, weapons,
                                                               weapon_index =
                                                                   pending
                                                                       .weapon_index,
                                                               close](
                                                                  const ::ui::
                                                                      ActivationEvent
                                                                          &) {
                                                                if (action ==
                                                                    LoadoutValue::Action::Buy) {
                                                                  weapons.buy(
                                                                      weapon_index);
                                                                } else if (
                                                                    action ==
                                                                    LoadoutValue::Action::Equip) {
                                                                  weapons.equip(
                                                                      weapon_index);
                                                                }
                                                                if (close)
                                                                  close();
                                                              },
                                                      },
                                                      components::Button),
                                                  ::ui::component(
                                                      "Button",
                                                      components::ButtonProps{
                                                          .key = "cancel",
                                                          .id = "CancelLoadoutActio"
                                                                "nButton",
                                                          .label = "Cancel",
                                                          .on_activate =
                                                              [close](
                                                                  const ::ui::
                                                                      ActivationEvent
                                                                          &) {
                                                                if (close)
                                                                  close();
                                                              },
                                                      },
                                                      components::Button),
                                              }),
                                          },
                                          components::Box),
                                  }),
                          },
                          components::Box),
                  }),
      },
      components::Dialog);
}

} // namespace

::ui::UiElement LoadoutConfirmDialog(const LoadoutConfirmDialogProps &props) {
  (void)props;
  LoadoutValue::PendingAction pending = use_loadout().pending;
  if (pending.action == LoadoutValue::Action::None)
    return ::ui::empty();

  return ::ui::component("LoadoutConfirmDialogBody", pending,
                         LoadoutConfirmDialogBody,
                         confirm_body_key(pending.generation));
}

} // namespace shooter
