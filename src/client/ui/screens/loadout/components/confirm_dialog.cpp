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

const char *confirm_body_key(::ui::retained::UiElementFrame &frame,
                             uint32_t generation) {
  char key[64] = {};
  snprintf(key, sizeof(key), "confirm-%u", generation);
  return frame.copy_string(key);
}

::ui::retained::UiElement
render_loadout_confirm_dialog_body(const LoadoutPendingAction &pending,
                                   ::ui::retained::UiElementFrame &frame) {
  ShooterWeaponRead weapon = use_weapon_read(pending.weapon_index);
  std::function<void()> buy = use_buy_weapon(pending.weapon_index);
  std::function<void()> equip = use_equip_weapon(pending.weapon_index);
  std::function<void()> close = use_clear_pending_loadout_action();
  if (!weapon.valid)
    return frame.empty();

  namespace retained = ::ui::retained;
  namespace components = ::ui::components;
  const char *title = use_text_storage(
      "%s",
      pending.action == LOADOUT_ACTION_BUY ? "Confirm Buy" : "Confirm Equip");
  const char *message =
      pending.action == LOADOUT_ACTION_BUY
          ? use_text_storage("Buy %s for %d credits?", weapon.name, weapon.cost)
          : use_text_storage("Equip %s as active weapon?", weapon.name);
  int action = pending.action;

  return components::Box(
      frame,
      {
          .key = "scrim",
          .width = retained::Length::percent(100.0f),
          .height = retained::Length::percent(100.0f),
          .align_items = retained::AlignItems::Center,
          .justify_content = retained::JustifyContent::Center,
          .modal = true,
          .background = {0, 0, 0, 160},
          .children = frame.children({
              components::Box(
                  frame,
                  {
                      .key = "panel",
                      .width = retained::Length::points(360.0f),
                      .padding = {18.0f, 18.0f, 18.0f, 18.0f},
                      .gap = 12.0f,
                      .background = {18, 26, 32, 255},
                      .border = {92, 116, 126, 255},
                      .border_width = 1.0f,
                      .children = frame.children({
                          components::Text(
                              frame,
                              {
                                  .key = "title",
                                  .value = title,
                                  .height = retained::Length::points(24.0f),
                                  .text_color = {240, 248, 244, 255},
                                  .font_size = 22,
                              }),
                          components::Text(
                              frame,
                              {
                                  .key = "message",
                                  .value = message,
                                  .height = retained::Length::points(18.0f),
                                  .text_color = {202, 218, 216, 255},
                                  .font_size = 15,
                              }),
                          components::Box(
                              frame,
                              {
                                  .key = "actions",
                                  .direction = retained::FlexDirection::Row,
                                  .gap = 10.0f,
                                  .children = frame.children({
                                      components::Button(
                                          frame,
                                          {
                                              .key = "confirm",
                                              .id =
                                                  "ConfirmLoadoutActionButton",
                                              .label = "Confirm",
                                              .initial_focus = true,
                                              .on_confirm =
                                                  [action, buy, equip, close] {
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
                                      components::Button(
                                          frame,
                                          {
                                              .key = "cancel",
                                              .id = "CancelLoadoutActionButton",
                                              .label = "Cancel",
                                              .on_confirm = close,
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

::ui::retained::UiElement
render_loadout_confirm_dialog(const LoadoutConfirmDialogProps &props,
                              ::ui::retained::UiElementFrame &frame) {
  (void)props;
  LoadoutPendingAction pending = use_pending_loadout_action();
  if (pending.action == LOADOUT_ACTION_NONE)
    return frame.empty();

  return frame.component("LoadoutConfirmDialogBody", pending,
                         render_loadout_confirm_dialog_body,
                         confirm_body_key(frame, pending.generation));
}

} // namespace

::ui::retained::UiElement
LoadoutConfirmDialog(::ui::retained::UiElementFrame &frame) {
  return frame.component("LoadoutConfirmDialog", LoadoutConfirmDialogProps{},
                         render_loadout_confirm_dialog);
}

} // namespace shooter
