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

static const char *screen_entry_key(::ui::retained::UiElementFrame &frame,
                                    const char *prefix,
                                    client::ui::UiScreenEntryId entry_id) {
  char key[64] = {};
  snprintf(key, sizeof(key), "%s-%u", prefix, entry_id);
  return frame.copy_string(key);
}

static ::ui::retained::UiElement
render_main_menu_screen(const MainMenuScreenProps &props,
                        ::ui::retained::UiElementFrame &frame) {
  (void)props;
  std::function<void()> start_match = use_start_match();
  std::function<void()> open_options = use_push_options_screen();
  std::function<void()> request_quit = client::ui::use_request_quit();
  namespace retained = ::ui::retained;
  namespace components = ::ui::components;

  return components::Box(
      frame,
      {
          .key = "root",
          .style =
              {
                  .width = retained::Length::percent(100.0f),
                  .height = retained::Length::percent(100.0f),
                  .direction = retained::FlexDirection::Column,
                  .align_items = retained::AlignItems::Center,
                  .justify_content = retained::JustifyContent::Center,
                  .padding = {36.0f, 36.0f, 36.0f, 36.0f},
                  .background = {8, 14, 18, 255},
              },
          .children = frame.children({
              components::Box(
                  frame,
                  {
                      .key = "panel",
                      .style =
                          {
                              .width = retained::Length::points(340.0f),
                              .direction = retained::FlexDirection::Column,
                              .align_items = retained::AlignItems::Center,
                              .padding = {24.0f, 24.0f, 24.0f, 24.0f},
                              .gap = 14.0f,
                              .background = {18, 27, 32, 245},
                              .border = {83, 108, 118, 255},
                              .border_width = 1.0f,
                          },
                      .children = frame.children({
                          components::Text(
                              frame,
                              {
                                  .key = "title",
                                  .value = "Reference Shooter",
                                  .style =
                                      {
                                          .height =
                                              retained::Length::points(36.0f),
                                          .text = {235, 246, 242, 255},
                                          .font_size = 30,
                                      },
                              }),
                          components::Text(
                              frame,
                              {
                                  .key = "subtitle",
                                  .value = "SDL3 / retained UI flow",
                                  .style =
                                      {
                                          .height =
                                              retained::Length::points(22.0f),
                                          .text = {154, 177, 184, 255},
                                          .font_size = 16,
                                      },
                              }),
                          components::Button(frame,
                                             {
                                                 .key = "start",
                                                 .id = "StartMatchButton",
                                                 .label = "Start Match",
                                                 .on_activate =
                                                     [start_match](
                                                         const retained::
                                                             ActivationEvent &) {
                                                       if (start_match)
                                                         start_match();
                                                     },
                                             }),
                          components::Button(
                              frame,
                              {
                                  .key = "options",
                                  .id = "OpenOptionsFromMainMenuButton",
                                  .label = "Options",
                                  .on_activate =
                                      [open_options](
                                          const retained::ActivationEvent &) {
                                        if (open_options)
                                          open_options();
                                      },
                              }),
                          components::Button(frame,
                                             {
                                                 .key = "quit",
                                                 .id = "QuitButton",
                                                 .disabled = !request_quit,
                                                 .label = "Quit",
                                                 .on_activate =
                                                     [request_quit](
                                                         const retained::
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

bool MainMenuScreen::build_element(::ui::retained::UiElementFrame &frame,
                                   ::ui::retained::UiElement *out) {
  if (!out)
    return false;
  *out = frame.component("MainMenuScreen", MainMenuScreenProps{},
                         render_main_menu_screen,
                         screen_entry_key(frame, "main-menu", entry_id()));
  return true;
}

void MainMenuScreen::build_ui() {}

} // namespace shooter
