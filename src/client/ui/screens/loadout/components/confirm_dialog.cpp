#include "confirm_dialog.h"

#include <functional>
#include <stdint.h>
#include <stdio.h>

#include "../../../../../react.h"
#include "../../../../../ui/components/components.h"
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

::ui::UiElement
render_loadout_confirm_dialog_body(const LoadoutPendingAction &pending) {
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

  return components::Dialog(
      {
          .key = "scrim",
          .style =
              {
                  .width = ::ui::Length::percent(100.0f),
                  .height = ::ui::Length::percent(100.0f),
                  .align_items = ::ui::AlignItems::Center,
                  .justify_content = ::ui::JustifyContent::Center,
                  .background = {0, 0, 0, 160},
              },
          .children =
              ::ui::children(
                  {
                      components::Box(
                          {
                              .key = "panel",
                              .style =
                                  {
                                      .width = ::ui::Length::points(360.0f),
                                      .padding = {18.0f, 18.0f, 18.0f, 18.0f},
                                      .gap = 12.0f,
                                      .background = {18, 26, 32, 255},
                                      .border = {92, 116, 126, 255},
                                      .border_width = 1.0f,
                                  },
                              .children = ::ui::children(
                                  {
                                      components::Text(
                                          {
                                              .key = "title",
                                              .value = title,
                                              .style =
                                                  {
                                                      .height =
                                                          ::ui::Length::points(
                                                              24.0f),
                                                      .text = {240, 248, 244,
                                                               255},
                                                      .font_size = 22,
                                                  },
                                          }),
                                      components::Text(
                                          {
                                              .key = "message",
                                              .value = message,
                                              .style =
                                                  {
                                                      .height =
                                                          ::ui::Length::points(
                                                              18.0f),
                                                      .text = {202, 218, 216,
                                                               255},
                                                      .font_size = 15,
                                                  },
                                          }),
                                      components::Box(
                                          {
                                              .key = "actions",
                                              .style =
                                                  {
                                                      .direction =
                                                          ::ui::FlexDirection::
                                                              Row,
                                                      .gap = 10.0f,
                                                  },
                                              .children = ::ui::children({
                                                  components::Button({
                                                      .key = "confirm",
                                                      .id = "ConfirmLoadoutActi"
                                                            "onButton",
                                                      .autofocus = true,
                                                      .label = "Confirm",
                                                      .on_activate =
                                                          [action, buy, equip,
                                                           close](
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
                                                  }),
                                                  components::Button({
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
                                                  }),
                                              }),
                                          }),
                                  }),
                          }),
                  }),
      });
}

struct LoadoutConfirmDialogProps {
  uint32_t unused = 0;
};

::ui::UiElement
render_loadout_confirm_dialog(const LoadoutConfirmDialogProps &props) {
  (void)props;
  LoadoutPendingAction pending = use_pending_loadout_action();
  if (pending.action == LOADOUT_ACTION_NONE)
    return ::ui::empty();

  return ::ui::component("LoadoutConfirmDialogBody", pending,
                         render_loadout_confirm_dialog_body,
                         confirm_body_key(pending.generation));
}

} // namespace

::ui::UiElement LoadoutConfirmDialog() {
  return ::ui::component("LoadoutConfirmDialog", LoadoutConfirmDialogProps{},
                         render_loadout_confirm_dialog);
}

} // namespace shooter
