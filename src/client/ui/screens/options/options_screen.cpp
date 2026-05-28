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

static const char *screen_entry_key(::ui::retained::UiElementFrame &frame,
                                    const char *prefix,
                                    client::ui::UiScreenEntryId entry_id) {
  char key[64] = {};
  snprintf(key, sizeof(key), "%s-%u", prefix, entry_id);
  return frame.copy_string(key);
}

static ::ui::retained::UiElement
render_options_screen(const OptionsScreenProps &props,
                      ::ui::retained::UiElementFrame &frame) {
  (void)props;
  client::ui::ScreenNavigator nav = client::ui::use_screen_navigator();
  bool is_top = client::ui::use_screen_is_top();
  int *large_hud = use_state_int(0);
  int *reduced_motion = use_state_int(1);
  std::string *player_name = use_state<std::string>(std::string("Ace"));
  namespace retained = ::ui::retained;
  namespace components = ::ui::components;

  if (!is_top || !player_name)
    return frame.empty();

  return components::Dialog(
      frame,
      {
          .key = "root",
          .style =
              {
                  .width = retained::Length::percent(100.0f),
                  .height = retained::Length::percent(100.0f),
                  .direction = retained::FlexDirection::Column,
                  .align_items = retained::AlignItems::Start,
                  .padding = {24.0f, 24.0f, 24.0f, 24.0f},
                  .gap = 12.0f,
                  .background = {14, 22, 28, 245},
              },
          .children = frame.children({
              components::Text(
                  frame,
                  {
                      .key = "title",
                      .value = "Options",
                      .style =
                          {
                              .height = retained::Length::points(32.0f),
                              .text = {236, 246, 242, 255},
                              .font_size = 26,
                          },
                  }),
              components::Checkbox(frame,
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
                  frame,
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
              components::Button(
                  frame,
                  {
                      .key = "back",
                      .id = "BackFromOptionsButton",
                      .label = "Back",
                      .on_activate =
                          [pop = nav.pop_current](
                              const retained::ActivationEvent &) {
                            if (pop)
                              pop();
                          },
                  }),
              components::Input(
                  frame,
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

bool OptionsScreen::build_element(::ui::retained::UiElementFrame &frame,
                                  ::ui::retained::UiElement *out) {
  if (!out)
    return false;
  *out = frame.component("OptionsScreen", OptionsScreenProps{},
                         render_options_screen,
                         screen_entry_key(frame, "options", entry_id()));
  return true;
}

void OptionsScreen::build_ui() {}

} // namespace shooter
