#include "confirm_dialog.h"

#include <functional>
#include <stdint.h>
#include <stdio.h>

#include "../../../../../react.h"
#include "../../../../../ui/components/components.h"
#include "client/ui/components/tokens.h"
#include "../../../hooks/shooter_weapons.h"
#include "../../../providers/shooter_provider.h"
#include "../loadout_state.h"

namespace shooter {

namespace {

const char *confirm_body_key(uint32_t generation) {
  char key[64] = {};
  snprintf(key, sizeof(key), "confirm-%u", generation);
  return ::ui::copy_string(key);
}

::ui::UiElement LoadoutConfirmDialogBody(const LoadoutPendingAction &pending) {
  ShooterWeaponRead weapon = use_weapon_read(pending.weapon_index);
  std::function<void()> buy = use_buy_weapon(pending.weapon_index);
  std::function<void()> equip = use_equip_weapon(pending.weapon_index);
  std::function<void()> close = use_clear_pending_loadout_action();
  if (!weapon.valid)
    return ::ui::empty();

  namespace components = ::ui::components;
  const char *title = use_text_storage(
      "%s",
      pending.action == LOADOUT_ACTION_BUY ? "Confirm Buy" : "Confirm Equip");
  const char *message =
      pending.action == LOADOUT_ACTION_BUY
          ? use_text_storage("Buy %s for %d credits?", weapon.name, weapon.cost)
          : use_text_storage("Equip %s as active weapon?", weapon.name);
  int action = pending.action;

  return ::ui::component(
      "Dialog",
      components::DialogProps{
          .key = "scrim",
          .style =
              {
                  .align_items = ::ui::AlignItems::Center,
                  .justify_content = ::ui::JustifyContent::Center,
                  .width = ::ui::Length::percent(100.0f),
                  .height = ::ui::Length::percent(100.0f),
              },
          .visual = tokens::fill_visual({0, 0, 0, 160}),
          .children =
              ::ui::children(
                  {
                      ::ui::component(
                          "Box",
                          components::BoxProps{
                              .key = "panel",
                              .style =
                                  {
                                      .width = ::ui::Length::points(360.0f),
                                      .padding = {18.0f, 18.0f, 18.0f, 18.0f},
                                      .gap = 12.0f,
                                      .border_width = 1.0f,
                                  },
                              .visual = tokens::panel_visual({18, 26, 32, 255},
                                                            {92, 116, 126, 255}),
                              .children = ::ui::children(
                                  {
                                      ::ui::component(
                                          "Text",
                                          components::TextProps{
                                              .key = "title",
                                              .value = title,
                                              .style =
                                                  {
                                                      .height =
                                                          ::ui::Length::points(
                                                              24.0f),
                                                  },
                                              .visual = tokens::text_visual(
                                                  {240, 248, 244, 255}, 22),
                                          },
                                          components::Text),
                                      ::ui::component(
                                          "Text",
                                          components::TextProps{
                                              .key = "message",
                                              .value = message,
                                              .style =
                                                  {
                                                      .height =
                                                          ::ui::Length::points(
                                                              18.0f),
                                                  },
                                              .visual = tokens::text_visual(
                                                  {202, 218, 216, 255}, 15),
                                          },
                                          components::Text),
                                      ::ui::component(
                                          "Box",
                                          components::BoxProps{
                                              .key = "actions",
                                              .style =
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
                                                              [action, buy,
                                                               equip, close](
                                                                  const ::ui::
                                                                      ActivationEvent
                                                                          &) {
                                                                if (action ==
                                                                    LOADOUT_ACTION_BUY) {
                                                                  if (buy)
                                                                    buy();
                                                                } else if (
                                                                    action ==
                                                                    LOADOUT_ACTION_EQUIP) {
                                                                  if (equip)
                                                                    equip();
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
  LoadoutPendingAction pending = use_pending_loadout_action();
  if (pending.action == LOADOUT_ACTION_NONE)
    return ::ui::empty();

  return ::ui::component("LoadoutConfirmDialogBody", pending,
                         LoadoutConfirmDialogBody,
                         confirm_body_key(pending.generation));
}

} // namespace shooter
