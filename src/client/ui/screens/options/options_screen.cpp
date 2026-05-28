#include "options_screen.h"

#include <memory>
#include <stdio.h>
#include <string>

#include "../../../../react.h"
#include "../../../../ui/components/components.h"
#include "../../callback_deps.h"
#include "../../client_ui.h"

namespace shooter {

std::function<void()> use_push_options_screen() {
  client::ui::ScreenNavigator nav = client::ui::use_screen_navigator();
  return use_callback(
      [nav] {
        if (nav.push)
          nav.push(std::make_unique<OptionsScreen>());
      },
      client::ui::callback_deps(nav.current_entry_id));
}

struct OptionsScreenProps {
  uint32_t unused = 0;
};

static const char *screen_entry_key(const char *prefix,
                                    client::ui::UiScreenEntryId entry_id) {
  char key[64] = {};
  snprintf(key, sizeof(key), "%s-%u", prefix, entry_id);
  return ::ui::copy_string(key);
}

static ::ui::UiElement render_options_screen(const OptionsScreenProps &props) {
  (void)props;
  client::ui::ScreenNavigator nav = client::ui::use_screen_navigator();
  bool is_top = client::ui::use_screen_is_top();
  int *large_hud = use_state_int(0);
  int *reduced_motion = use_state_int(1);
  std::string *player_name = use_state<std::string>(std::string("Ace"));
  namespace components = ::ui::components;

  if (!is_top || !player_name)
    return ::ui::empty();

  return components::Dialog(
      {
          .key = "root",
          .style =
              {
                  .width = ::ui::Length::percent(100.0f),
                  .height = ::ui::Length::percent(100.0f),
                  .direction = ::ui::FlexDirection::Column,
                  .align_items = ::ui::AlignItems::Start,
                  .padding = {24.0f, 24.0f, 24.0f, 24.0f},
                  .gap = 12.0f,
                  .background = {14, 22, 28, 245},
              },
          .children =
              ::ui::children(
                  {
                      components::Text({
                          .key = "title",
                          .value = "Options",
                          .style =
                              {
                                  .height = ::ui::Length::points(32.0f),
                                  .text = {236, 246, 242, 255},
                                  .font_size = 26,
                              },
                      }),
                      components::Checkbox(
                          {
                              .key = "large-hud",
                              .id = "LargeHudToggle",
                              .checked = large_hud && *large_hud != 0,
                              .label = "Large HUD",
                              .on_change =
                                  [large_hud](bool enabled) {
                                    if (large_hud)
                                      *large_hud = enabled ? 1 : 0;
                                  },
                          }),
                      components::Checkbox(
                          {
                              .key = "reduced-motion",
                              .id = "ReducedMotionToggle",
                              .checked = reduced_motion && *reduced_motion != 0,
                              .label = "Reduce Motion",
                              .on_change =
                                  [reduced_motion](bool enabled) {
                                    if (reduced_motion)
                                      *reduced_motion = enabled ? 1 : 0;
                                  },
                          }),
                      components::Button({
                          .key = "back",
                          .id = "BackFromOptionsButton",
                          .label = "Back",
                          .on_activate =
                              [pop = nav.pop_current](
                                  const ::ui::ActivationEvent &) {
                                if (pop)
                                  pop();
                              },
                      }),
                      components::Input(
                          {
                              .key = "name",
                              .id = "NameInput",
                              .accessibility =
                                  {
                                      .label = "Name",
                                  },
                              .value = player_name->c_str(),
                              .on_change =
                                  [player_name](const std::string &value) {
                                    if (player_name)
                                      *player_name = value;
                                  },
                          }),
                  }),
      });
}

bool OptionsScreen::build_element(::ui::UiElementFrame &frame,
                                  ::ui::UiElement *out) {
  if (!out)
    return false;
  *out = ::ui::component("OptionsScreen", OptionsScreenProps{},
                         render_options_screen,
                         screen_entry_key("options", entry_id()));
  return true;
}

void OptionsScreen::build_ui() {}

} // namespace shooter
