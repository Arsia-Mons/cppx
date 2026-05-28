#include "in_game_screen.h"

#include <memory>
#include <stdio.h>

#include "../../../../react.h"
#include "../../../../ui/components/components.h"
#include "../../callback_deps.h"
#include "../../client_ui.h"
#include "../../components/hud_band.h"
#include "../../internal/deferred_ui_mutation.h"
#include "../../providers/shooter_provider.h"
#include "../loadout/loadout_screen.h"
#include "../pause/pause_screen.h"

namespace shooter {

std::function<void()> use_start_match() {
  ShooterGame *game = use_shooter_game();
  client::ui::ScreenNavigator nav = client::ui::use_screen_navigator();
  client::ui::internal::DeferredUiMutationSink mutations =
      client::ui::internal::use_deferred_ui_mutations();
  return use_callback(
      [game, nav, mutations] {
        if (!nav.reset_to)
          return;
        if (game && mutations) {
          mutations.submit([game] { game->reset(); });
        }
        nav.reset_to(std::make_unique<ShooterGameScreen>());
      },
      client::ui::callback_deps(
          client::ui::callback_deps_ptr(game),
          client::ui::callback_deps_ptr(mutations.owner()),
          nav.current_entry_id));
}

struct ShooterGameScreenProps {
  uint32_t unused = 0;
};

static const char *screen_entry_key(::ui::retained::UiElementFrame &frame,
                                    const char *prefix,
                                    client::ui::UiScreenEntryId entry_id) {
  char key[64] = {};
  snprintf(key, sizeof(key), "%s-%u", prefix, entry_id);
  return frame.copy_string(key);
}

static ::ui::retained::UiElement
render_shooter_game_screen(const ShooterGameScreenProps &props,
                           ::ui::retained::UiElementFrame &frame) {
  (void)props;
  bool is_top = client::ui::use_screen_is_top();
  std::function<void()> open_pause = use_push_pause_screen();
  std::function<void()> open_loadout = use_push_loadout_screen();
  namespace retained = ::ui::retained;
  namespace components = ::ui::components;

  if (!is_top)
    return frame.empty();

  return components::Box(
      frame,
      {
          .key = "root",
          .style =
              {
                  .width = retained::Length::percent(100.0f),
                  .height = retained::Length::percent(100.0f),
                  .direction = retained::FlexDirection::Column,
                  .padding = {24.0f, 24.0f, 24.0f, 24.0f},
                  .gap = 18.0f,
                  .background = {10, 16, 18, 255},
              },
          .children = frame.children({
              HudBand(frame),
              components::Box(
                  frame,
                  {
                      .key = "actions",
                      .style =
                          {
                              .direction = retained::FlexDirection::Row,
                              .align_items = retained::AlignItems::Start,
                              .gap = 12.0f,
                          },
                      .children = frame.children({
                          components::Button(
                              frame,
                              {
                                  .key = "pause",
                                  .id = "OpenPauseButton",
                                  .label = "Pause",
                                  .on_activate =
                                      [open_pause](
                                          const retained::ActivationEvent &) {
                                        if (open_pause)
                                          open_pause();
                                      },
                              }),
                          components::Button(
                              frame,
                              {
                                  .key = "loadout",
                                  .id = "OpenLoadoutButton",
                                  .label = "Loadout",
                                  .on_activate =
                                      [open_loadout](
                                          const retained::ActivationEvent &) {
                                        if (open_loadout)
                                          open_loadout();
                                      },
                              }),
                      }),
                  }),
          }),
      });
}

bool ShooterGameScreen::build_element(::ui::retained::UiElementFrame &frame,
                                      ::ui::retained::UiElement *out) {
  if (!out)
    return false;
  *out = frame.component("ShooterGameScreen", ShooterGameScreenProps{},
                         render_shooter_game_screen,
                         screen_entry_key(frame, "shooter-game", entry_id()));
  return true;
}

void ShooterGameScreen::build_ui() {}

} // namespace shooter
