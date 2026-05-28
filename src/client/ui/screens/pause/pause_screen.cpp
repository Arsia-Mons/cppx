#include "pause_screen.h"

#include <memory>
#include <stdio.h>

#include "../../../../react.h"
#include "../../../../ui/components/components.h"
#include "../../callback_deps.h"
#include "../../client_ui.h"
#include "../loadout/loadout_screen.h"
#include "../main_menu/main_menu_screen.h"
#include "../options/options_screen.h"

namespace shooter {

std::function<void()> use_push_pause_screen() {
  client::ui::ScreenNavigator nav = client::ui::use_screen_navigator();
  return use_callback(
      [nav] {
        if (nav.push)
          nav.push(std::make_unique<PauseScreen>());
      },
      client::ui::callback_deps(nav.current_entry_id));
}

struct PauseScreenProps {
  uint32_t unused = 0;
};

static const char *screen_entry_key(const char *prefix,
                                    client::ui::UiScreenEntryId entry_id) {
  char key[64] = {};
  snprintf(key, sizeof(key), "%s-%u", prefix, entry_id);
  return ::ui::copy_string(key);
}

static ::ui::UiElement PauseScreenView(const PauseScreenProps &props) {
  (void)props;
  client::ui::ScreenNavigator nav = client::ui::use_screen_navigator();
  bool is_top = client::ui::use_screen_is_top();
  std::function<void()> open_options = use_push_options_screen();
  std::function<void()> open_loadout = use_push_loadout_screen();
  std::function<void()> exit_to_main_menu = use_exit_to_main_menu();
  namespace components = ::ui::components;

  if (!is_top)
    return ::ui::empty();

  return ::ui::components::elements::Dialog(
      {
          .key = "root",
          .style =
              {
                  .width = ::ui::Length::percent(100.0f),
                  .height = ::ui::Length::percent(100.0f),
                  .direction = ::ui::FlexDirection::Column,
                  .align_items = ::ui::AlignItems::Center,
                  .justify_content = ::ui::JustifyContent::Center,
              },
          .children =
              ::ui::children(
                  {
                      ::ui::components::elements::Box(
                          {
                              .key = "panel",
                              .style =
                                  {
                                      .width = ::ui::Length::points(220.0f),
                                      .direction = ::ui::FlexDirection::Column,
                                      .align_items = ::ui::AlignItems::Center,
                                      .padding = {18.0f, 18.0f, 18.0f, 18.0f},
                                      .gap = 12.0f,
                                      .background = {17, 24, 30, 255},
                                      .border = {78, 96, 108, 255},
                                      .border_width = 1.0f,
                                  },
                              .children =
                                  ::ui::children(
                                      {
                                          ::ui::components::elements::Text(
                                              {
                                                  .key = "title",
                                                  .value = "Paused",
                                                  .style =
                                                      {
                                                          .height =
                                                              ::ui::Length::points(
                                                                  34.0f),
                                                          .text = {236, 246,
                                                                   242, 255},
                                                          .font_size = 28,
                                                      },
                                              }),
                                          ::ui::components::elements::Button({
                                              .key = "resume",
                                              .id = "ResumeButton",
                                              .label = "Resume",
                                              .on_activate =
                                                  [pop = nav.pop_current](
                                                      const ::ui::
                                                          ActivationEvent &) {
                                                    if (pop)
                                                      pop();
                                                  },
                                          }),
                                          ::ui::components::elements::Button({
                                              .key = "options",
                                              .id =
                                                  "OpenOptionsFromPauseButton",
                                              .label = "Options",
                                              .on_activate =
                                                  [open_options](
                                                      const ::ui::
                                                          ActivationEvent &) {
                                                    if (open_options)
                                                      open_options();
                                                  },
                                          }),
                                          ::ui::components::elements::Button({
                                              .key = "loadout",
                                              .id =
                                                  "OpenLoadoutFromPauseButton",
                                              .label = "Loadout",
                                              .on_activate =
                                                  [open_loadout](
                                                      const ::ui::
                                                          ActivationEvent &) {
                                                    if (open_loadout)
                                                      open_loadout();
                                                  },
                                          }),
                                          ::ui::components::elements::Button({
                                              .key = "exit-to-menu",
                                              .id = "ExitToMainMenuButton",
                                              .label = "Exit To Menu",
                                              .on_activate =
                                                  [exit_to_main_menu](
                                                      const ::ui::
                                                          ActivationEvent &) {
                                                    if (exit_to_main_menu)
                                                      exit_to_main_menu();
                                                  },
                                          }),
                                      }),
                          }),
                  }),
      });
}

bool PauseScreen::build_element(::ui::UiElementFrame &frame,
                                ::ui::UiElement *out) {
  if (!out)
    return false;
  *out = ::ui::component("PauseScreen", PauseScreenProps{}, PauseScreenView,
                         screen_entry_key("pause", entry_id()));
  return true;
}

void PauseScreen::build_ui() {}

} // namespace shooter
