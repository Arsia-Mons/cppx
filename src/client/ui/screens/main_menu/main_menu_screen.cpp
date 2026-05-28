#include "main_menu_screen.h"

#include <memory>
#include <stdio.h>

#include "../../../../react.h"
#include "../../../../ui/components/components.h"
#include "../../callback_deps.h"
#include "../../client_ui.h"
#include "../../providers/app_shell.h"
#include "../in_game/in_game_screen.h"
#include "../options/options_screen.h"

namespace shooter {

std::function<void()> use_exit_to_main_menu() {
  client::ui::ScreenNavigator nav = client::ui::use_screen_navigator();
  return use_callback(
      [nav] {
        if (nav.reset_to)
          nav.reset_to(std::make_unique<MainMenuScreen>());
      },
      client::ui::callback_deps(nav.current_entry_id));
}

struct MainMenuScreenProps {
  uint32_t unused = 0;
};

static const char *screen_entry_key(const char *prefix,
                                    client::ui::UiScreenEntryId entry_id) {
  char key[64] = {};
  snprintf(key, sizeof(key), "%s-%u", prefix, entry_id);
  return ::ui::copy_string(key);
}

static ::ui::UiElement
render_main_menu_screen(const MainMenuScreenProps &props) {
  (void)props;
  std::function<void()> start_match = use_start_match();
  std::function<void()> open_options = use_push_options_screen();
  std::function<void()> request_quit = client::ui::use_request_quit();
  namespace components = ::ui::components;

  return components::Box(
      {
          .key = "root",
          .style =
              {
                  .width = ::ui::Length::percent(100.0f),
                  .height = ::ui::Length::percent(100.0f),
                  .direction = ::ui::FlexDirection::Column,
                  .align_items = ::ui::AlignItems::Center,
                  .justify_content = ::ui::JustifyContent::Center,
                  .padding = {36.0f, 36.0f, 36.0f, 36.0f},
                  .background = {8, 14, 18, 255},
              },
          .children =
              ::ui::children(
                  {
                      components::Box(
                          {
                              .key = "panel",
                              .style =
                                  {
                                      .width = ::ui::Length::points(340.0f),
                                      .direction = ::ui::FlexDirection::Column,
                                      .align_items = ::ui::AlignItems::Center,
                                      .padding = {24.0f, 24.0f, 24.0f, 24.0f},
                                      .gap = 14.0f,
                                      .background = {18, 27, 32, 245},
                                      .border = {83, 108, 118, 255},
                                      .border_width = 1.0f,
                                  },
                              .children =
                                  ::ui::children(
                                      {
                                          components::Text(
                                              {
                                                  .key = "title",
                                                  .value = "Reference Shooter",
                                                  .style =
                                                      {
                                                          .height =
                                                              ::ui::Length::points(
                                                                  36.0f),
                                                          .text = {235, 246,
                                                                   242, 255},
                                                          .font_size = 30,
                                                      },
                                              }),
                                          components::Text(
                                              {
                                                  .key = "subtitle",
                                                  .value =
                                                      "SDL3 / retained UI flow",
                                                  .style =
                                                      {
                                                          .height =
                                                              ::ui::Length::points(
                                                                  22.0f),
                                                          .text = {154, 177,
                                                                   184, 255},
                                                          .font_size = 16,
                                                      },
                                              }),
                                          components::Button({
                                              .key = "start",
                                              .id = "StartMatchButton",
                                              .label = "Start Match",
                                              .on_activate =
                                                  [start_match](
                                                      const ::ui::
                                                          ActivationEvent &) {
                                                    if (start_match)
                                                      start_match();
                                                  },
                                          }),
                                          components::Button({
                                              .key = "options",
                                              .id = "OpenOptionsFromMainMenuBut"
                                                    "ton",
                                              .label = "Options",
                                              .on_activate =
                                                  [open_options](
                                                      const ::ui::
                                                          ActivationEvent &) {
                                                    if (open_options)
                                                      open_options();
                                                  },
                                          }),
                                          components::Button({
                                              .key = "quit",
                                              .id = "QuitButton",
                                              .disabled = !request_quit,
                                              .label = "Quit",
                                              .on_activate =
                                                  [request_quit](
                                                      const ::ui::
                                                          ActivationEvent &) {
                                                    if (request_quit)
                                                      request_quit();
                                                  },
                                          }),
                                      }),
                          }),
                  }),
      });
}

bool MainMenuScreen::build_element(::ui::UiElementFrame &frame,
                                   ::ui::UiElement *out) {
  if (!out)
    return false;
  *out = ::ui::component("MainMenuScreen", MainMenuScreenProps{},
                         render_main_menu_screen,
                         screen_entry_key("main-menu", entry_id()));
  return true;
}

void MainMenuScreen::build_ui() {}

} // namespace shooter
