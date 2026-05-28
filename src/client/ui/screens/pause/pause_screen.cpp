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

static const char *screen_entry_key(::ui::retained::UiElementFrame &frame,
                                    const char *prefix,
                                    client::ui::UiScreenEntryId entry_id) {
  char key[64] = {};
  snprintf(key, sizeof(key), "%s-%u", prefix, entry_id);
  return frame.copy_string(key);
}

static ::ui::retained::UiElement
render_pause_screen(const PauseScreenProps &props,
                    ::ui::retained::UiElementFrame &frame) {
  (void)props;
  client::ui::ScreenNavigator nav = client::ui::use_screen_navigator();
  bool is_top = client::ui::use_screen_is_top();
  std::function<void()> open_options = use_push_options_screen();
  std::function<void()> open_loadout = use_push_loadout_screen();
  std::function<void()> exit_to_main_menu = use_exit_to_main_menu();
  namespace retained = ::ui::retained;
  namespace components = ::ui::components;

  if (!is_top)
    return frame.empty();

  return components::Box(
      frame,
      {
          .key = "root",
          .width = retained::Length::percent(100.0f),
          .height = retained::Length::percent(100.0f),
          .direction = retained::FlexDirection::Column,
          .align_items = retained::AlignItems::Center,
          .justify_content = retained::JustifyContent::Center,
          .modal = true,
          .children = frame.children({
              components::Box(
                  frame,
                  {
                      .key = "panel",
                      .width = retained::Length::points(220.0f),
                      .direction = retained::FlexDirection::Column,
                      .align_items = retained::AlignItems::Center,
                      .padding = {18.0f, 18.0f, 18.0f, 18.0f},
                      .gap = 12.0f,
                      .background = {17, 24, 30, 255},
                      .border = {78, 96, 108, 255},
                      .border_width = 1.0f,
                      .children = frame.children({
                          components::Text(
                              frame,
                              {
                                  .key = "title",
                                  .value = "Paused",
                                  .height = retained::Length::points(34.0f),
                                  .text_color = {236, 246, 242, 255},
                                  .font_size = 28,
                              }),
                          components::Button(frame,
                                             {
                                                 .key = "resume",
                                                 .id = "ResumeButton",
                                                 .label = "Resume",
                                                 .on_confirm = nav.pop_current,
                                             }),
                          components::Button(
                              frame,
                              {
                                  .key = "options",
                                  .id = "OpenOptionsFromPauseButton",
                                  .label = "Options",
                                  .on_confirm = open_options,
                              }),
                          components::Button(
                              frame,
                              {
                                  .key = "loadout",
                                  .id = "OpenLoadoutFromPauseButton",
                                  .label = "Loadout",
                                  .on_confirm = open_loadout,
                              }),
                          components::Button(
                              frame,
                              {
                                  .key = "exit-to-menu",
                                  .id = "ExitToMainMenuButton",
                                  .label = "Exit To Menu",
                                  .on_confirm = exit_to_main_menu,
                              }),
                      }),
                  }),
          }),
      });
}

bool PauseScreen::build_element(::ui::retained::UiElementFrame &frame,
                                ::ui::retained::UiElement *out) {
  if (!out)
    return false;
  *out = frame.component("PauseScreen", PauseScreenProps{}, render_pause_screen,
                         screen_entry_key(frame, "pause", entry_id()));
  return true;
}

void PauseScreen::build_ui() {}

} // namespace shooter
