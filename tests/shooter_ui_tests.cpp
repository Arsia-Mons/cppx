#include "client/ui/app_shell/client_ui.h"
#include "client/ui/app_shell/app_shell_provider.h"
#include "client/ui/app_theme.h"
#include "client/ui/providers/shooter_provider.h"
#include "ui/style/theme.h"
#include "ui/style/resolve.h"
#include "client/ui/screens/in_game/in_game_screen.h"
#include "client/ui/components/actions/app_button_variant.h"
#include "client/ui/screens/loadout/components/weapon_tile.h"
#include "client/ui/screens/loadout/loadout_screen.h"
#include "client/ui/screens/main_menu/main_menu_screen.h"
#include "client/ui/screens/options/options_screen.h"
#include "client/ui/screens/pause/pause_screen.h"
#include "client/ui/app_shell/ui_pipeline.h"
#include "game/shooter_game.h"
#include "react.h"

#include <functional>

#include <memory>
#include <stdio.h>
#include <string.h>

#define CHECK(expr)                                                            \
  do {                                                                         \
    if (!(expr)) {                                                             \
      fprintf(stderr, "CHECK failed at %s:%d: %s\n", __FILE__, __LINE__,       \
              #expr);                                                          \
      return false;                                                            \
    }                                                                          \
  } while (0)

struct TestFrameProviders {
  shooter::ShooterGame *game = nullptr;
  std::function<void()> request_quit = {};
};

static void run_pipeline_frame(client::ui::UiPipeline &pipeline,
                               const TestFrameProviders &providers,
                               const ::ui::UiInputFrame &input = {},
                               ::ui::Point pointer = {-1000.0f, -1000.0f},
                               const client::ui::RenderFrame &render = {}) {
  pipeline.set_frame_provider([&](::ui::UiElement child) {
    shooter::ShooterContextValue game_ctx{.game = providers.game};
    client::ui::AppShellContextValue shell_ctx{.request_quit =
                                                   providers.request_quit};
    return client::ui::ThemeProvider(::ui::children({
        shooter::ShooterProvider(
            game_ctx,
            ::ui::children({
                client::ui::AppShellProvider(shell_ctx,
                                             ::ui::children({child})),
            })),
    }));
  });
  pipeline.render_client_ui_frame(
      {
          .input = input,
          .layout = {800, 500},
          .pointer = pointer,
      },
      render);
  pipeline.set_frame_provider({});
}

static ::ui::NodeId find_retained_control(const ::ui::UiTree &tree,
                                          ::ui::NodeId id, const char *name,
                                          int offset = 0) {
  ::ui::NodeSnapshot node = {};
  if (!tree.snapshot(id, &node))
    return 0;
  if (strcmp(node.control_id ? node.control_id : "", name) == 0 &&
      node.control_offset == offset) {
    return id;
  }
  for (int i = 0; i < tree.child_count(id); ++i) {
    ::ui::NodeId found =
        find_retained_control(tree, tree.child_at(id, i), name, offset);
    if (found != 0)
      return found;
  }
  return 0;
}

static ::ui::NodeId retained_control_id(const client::ui::ClientUi &client_ui,
                                        const char *name, int offset = 0) {
  const ::ui::UiTree &tree = client_ui.retained_tree();
  return find_retained_control(tree, tree.root_id(), name, offset);
}

static ::ui::Point retained_center_of(const client::ui::ClientUi &client_ui,
                                      const char *name, int offset = 0) {
  const ::ui::UiTree &tree = client_ui.retained_tree();
  ::ui::NodeId id = retained_control_id(client_ui, name, offset);
  ::ui::NodeSnapshot node = {};
  if (id == 0 || !tree.snapshot(id, &node))
    return {-1000.0f, -1000.0f};
  return {
      node.layout.x + node.layout.width * 0.5f,
      node.layout.y + node.layout.height * 0.5f,
  };
}

static ::ui::UiInputFrame keyboard_confirm(void) {
  return {
      .confirm_pressed = true,
      .confirm_down = true,
      .source = ::ui::UiFocusSource::Keyboard,
  };
}

static ::ui::UiInputFrame keyboard_down(void) {
  return {
      .nav_down = true,
      .source = ::ui::UiFocusSource::Keyboard,
  };
}

static ::ui::UiInputFrame keyboard_right(void) {
  return {
      .nav_right = true,
      .source = ::ui::UiFocusSource::Keyboard,
  };
}

static ::ui::UiInputFrame keyboard_cancel(void) {
  return {
      .cancel_pressed = true,
      .cancel_down = true,
      .source = ::ui::UiFocusSource::Keyboard,
  };
}

static ::ui::UiInputFrame pointer_press(void) {
  return {
      .pointer_pressed = true,
      .pointer_down = true,
      .source = ::ui::UiFocusSource::Mouse,
  };
}

static ::ui::UiInputFrame pointer_release(void) {
  return {
      .pointer_released = true,
      .source = ::ui::UiFocusSource::Mouse,
  };
}

static bool shooter_game_buy_and_equip_are_real_state_writes(void) {
  shooter::ShooterGame game;

  CHECK(game.credits() == 450);
  CHECK(!game.weapon(1).owned);
  CHECK(game.buy_weapon(1));
  CHECK(game.credits() == 150);
  CHECK(game.weapon(1).owned);
  CHECK(game.equip_weapon(1));
  CHECK(game.selected_weapon() == 1);
  CHECK(game.weapon(1).equipped);
  CHECK(!game.can_buy_weapon(2));
  return true;
}

static bool main_menu_start_match_resets_game_and_stack(void) {
  react_init_runtime();
  shooter::ShooterGame game;
  CHECK(game.buy_weapon(1));
  CHECK(game.equip_weapon(1));
  CHECK(game.credits() == 150);
  CHECK(game.selected_weapon() == 1);

  bool quit_requested = false;
  TestFrameProviders providers{
      .game = &game,
      .request_quit = [&quit_requested] { quit_requested = true; },
  };
  client::ui::UiPipeline pipeline;
  client::ui::ClientUi &client_ui = pipeline.client_ui();
  CHECK(client_ui.push_screen(std::make_unique<shooter::MainMenuScreen>()));

  run_pipeline_frame(pipeline, providers);
  CHECK(client_ui.screens().count() == 1);
  CHECK(strcmp(client_ui.screens().top()->debug_name(), "MainMenu") == 0);

  run_pipeline_frame(pipeline, providers, keyboard_confirm());

  CHECK(client_ui.screens().count() == 1);
  CHECK(strcmp(client_ui.screens().top()->debug_name(), "ShooterGame") == 0);
  CHECK(game.credits() == 450);
  CHECK(game.selected_weapon() == 0);
  CHECK(game.weapon(0).equipped);
  CHECK(!game.weapon(1).owned);
  CHECK(!quit_requested);
  return true;
}

static bool main_menu_options_returns_to_menu_with_cancel(void) {
  react_init_runtime();
  shooter::ShooterGame game;
  TestFrameProviders providers{.game = &game};
  client::ui::UiPipeline pipeline;
  client::ui::ClientUi &client_ui = pipeline.client_ui();
  CHECK(client_ui.push_screen(std::make_unique<shooter::MainMenuScreen>()));

  run_pipeline_frame(pipeline, providers);
  run_pipeline_frame(pipeline, providers, keyboard_down());
  run_pipeline_frame(pipeline, providers, keyboard_confirm());

  CHECK(client_ui.screens().count() == 2);
  CHECK(strcmp(client_ui.screens().at(0)->debug_name(), "MainMenu") == 0);
  CHECK(strcmp(client_ui.screens().top()->debug_name(), "Options") == 0);

  run_pipeline_frame(pipeline, providers, keyboard_cancel());

  CHECK(client_ui.screens().count() == 1);
  CHECK(strcmp(client_ui.screens().top()->debug_name(), "MainMenu") == 0);
  return true;
}

static bool main_menu_quit_callback_runs(void) {
  react_init_runtime();
  shooter::ShooterGame game;
  bool quit_requested = false;
  TestFrameProviders providers{
      .game = &game,
      .request_quit = [&quit_requested] { quit_requested = true; },
  };
  client::ui::UiPipeline pipeline;
  client::ui::ClientUi &client_ui = pipeline.client_ui();
  CHECK(client_ui.push_screen(std::make_unique<shooter::MainMenuScreen>()));

  run_pipeline_frame(pipeline, providers);
  run_pipeline_frame(pipeline, providers, keyboard_down());
  run_pipeline_frame(pipeline, providers, keyboard_down());
  run_pipeline_frame(pipeline, providers, keyboard_confirm());

  CHECK(quit_requested);
  CHECK(client_ui.screens().count() == 1);
  CHECK(strcmp(client_ui.screens().top()->debug_name(), "MainMenu") == 0);
  return true;
}

static bool shooter_screen_pushes_pause_after_confirm(void) {
  react_init_runtime();
  shooter::ShooterGame game;
  TestFrameProviders providers{.game = &game};
  client::ui::UiPipeline pipeline;
  client::ui::ClientUi &client_ui = pipeline.client_ui();
  CHECK(client_ui.push_screen(std::make_unique<shooter::ShooterGameScreen>()));

  run_pipeline_frame(pipeline, providers);
  CHECK(client_ui.screens().count() == 1);
  CHECK(strcmp(client_ui.screens().top()->debug_name(), "ShooterGame") == 0);

  run_pipeline_frame(pipeline, providers, keyboard_confirm());

  CHECK(client_ui.screens().count() == 2);
  CHECK(strcmp(client_ui.screens().top()->debug_name(), "Pause") == 0);
  return true;
}

static bool pause_exit_to_main_menu_resets_stack(void) {
  react_init_runtime();
  shooter::ShooterGame game;
  TestFrameProviders providers{.game = &game};
  client::ui::UiPipeline pipeline;
  client::ui::ClientUi &client_ui = pipeline.client_ui();
  CHECK(client_ui.push_screen(std::make_unique<shooter::ShooterGameScreen>()));

  run_pipeline_frame(pipeline, providers);
  run_pipeline_frame(pipeline, providers, keyboard_confirm());
  CHECK(client_ui.screens().count() == 2);
  CHECK(strcmp(client_ui.screens().top()->debug_name(), "Pause") == 0);

  run_pipeline_frame(pipeline, providers);
  run_pipeline_frame(pipeline, providers, keyboard_down());
  run_pipeline_frame(pipeline, providers, keyboard_down());
  run_pipeline_frame(pipeline, providers, keyboard_down());
  run_pipeline_frame(pipeline, providers, keyboard_confirm());

  CHECK(client_ui.screens().count() == 1);
  CHECK(strcmp(client_ui.screens().top()->debug_name(), "MainMenu") == 0);
  return true;
}

static bool pause_options_returns_to_pause_through_screen_stack(void) {
  react_init_runtime();
  shooter::ShooterGame game;
  TestFrameProviders providers{.game = &game};
  client::ui::UiPipeline pipeline;
  client::ui::ClientUi &client_ui = pipeline.client_ui();
  CHECK(client_ui.push_screen(std::make_unique<shooter::ShooterGameScreen>()));

  run_pipeline_frame(pipeline, providers);
  run_pipeline_frame(pipeline, providers, keyboard_confirm());
  CHECK(client_ui.screens().count() == 2);
  CHECK(strcmp(client_ui.screens().top()->debug_name(), "Pause") == 0);

  run_pipeline_frame(pipeline, providers);
  run_pipeline_frame(pipeline, providers, keyboard_down());
  run_pipeline_frame(pipeline, providers, keyboard_confirm());
  CHECK(client_ui.screens().count() == 3);
  CHECK(strcmp(client_ui.screens().at(1)->debug_name(), "Pause") == 0);
  CHECK(strcmp(client_ui.screens().top()->debug_name(), "Options") == 0);

  run_pipeline_frame(pipeline, providers);
  run_pipeline_frame(pipeline, providers, keyboard_down());
  run_pipeline_frame(pipeline, providers, keyboard_down());
  run_pipeline_frame(pipeline, providers, keyboard_confirm());
  CHECK(client_ui.screens().count() == 2);
  CHECK(strcmp(client_ui.screens().top()->debug_name(), "Pause") == 0);
  return true;
}

static bool loadout_buy_uses_confirm_dialog_and_restores_parent_focus(void) {
  react_init_runtime();
  shooter::ShooterGame game;
  TestFrameProviders providers{.game = &game};
  client::ui::UiPipeline pipeline;
  client::ui::ClientUi &client_ui = pipeline.client_ui();
  CHECK(client_ui.push_screen(std::make_unique<shooter::LoadoutScreen>()));

  run_pipeline_frame(pipeline, providers);
  CHECK(::ui::focus_focused_id(client_ui.retained_focus()) ==
        retained_control_id(client_ui, shooter::WEAPON_TILE_CONTROL_ID, 0));
  CHECK(game.selected_weapon() == 0);

  run_pipeline_frame(pipeline, providers, keyboard_right());
  CHECK(::ui::focus_focused_id(client_ui.retained_focus()) ==
        retained_control_id(client_ui, shooter::WEAPON_TILE_CONTROL_ID, 1));
  // Focus is a pure-UI change: focusing tile 1 must NOT mutate game state.
  CHECK(game.selected_weapon() == 0);
  CHECK(!game.weapon(1).owned);
  CHECK(game.credits() == 450);

  run_pipeline_frame(pipeline, providers, keyboard_right());
  run_pipeline_frame(pipeline, providers, keyboard_down());
  ::ui::NodeId buy_id = retained_control_id(client_ui, "BuyWeaponButton");
  CHECK(::ui::focus_focused_id(client_ui.retained_focus()) == buy_id);

  run_pipeline_frame(pipeline, providers, keyboard_confirm());
  CHECK(!game.weapon(1).owned);
  CHECK(game.credits() == 450);

  run_pipeline_frame(pipeline, providers);
  CHECK(::ui::focus_focused_id(client_ui.retained_focus()) ==
        retained_control_id(client_ui, "ConfirmLoadoutActionButton"));
  CHECK(client_ui.retained_focus().previous_focus_before_modal == buy_id);

  run_pipeline_frame(pipeline, providers, keyboard_right());
  CHECK(::ui::focus_focused_id(client_ui.retained_focus()) ==
        retained_control_id(client_ui, "CancelLoadoutActionButton"));
  run_pipeline_frame(pipeline, providers, keyboard_confirm());
  CHECK(!game.weapon(1).owned);
  CHECK(game.credits() == 450);

  run_pipeline_frame(pipeline, providers);
  buy_id = retained_control_id(client_ui, "BuyWeaponButton");
  CHECK(::ui::focus_focused_id(client_ui.retained_focus()) == buy_id);

  run_pipeline_frame(pipeline, providers, keyboard_confirm());
  CHECK(!game.weapon(1).owned);
  run_pipeline_frame(pipeline, providers);
  CHECK(::ui::focus_focused_id(client_ui.retained_focus()) ==
        retained_control_id(client_ui, "ConfirmLoadoutActionButton"));
  run_pipeline_frame(pipeline, providers, keyboard_confirm());
  CHECK(game.weapon(1).owned);
  CHECK(game.credits() == 150);
  return true;
}

static bool loadout_tabs_and_equipment_slots_are_real_focus_targets(void) {
  react_init_runtime();
  shooter::ShooterGame game;
  TestFrameProviders providers{.game = &game};
  client::ui::UiPipeline pipeline;
  client::ui::ClientUi &client_ui = pipeline.client_ui();
  CHECK(client_ui.push_screen(std::make_unique<shooter::LoadoutScreen>()));

  run_pipeline_frame(pipeline, providers);
  CHECK(retained_control_id(client_ui, "WeaponsTab") != 0);
  CHECK(retained_control_id(client_ui, "GearTab") != 0);
  CHECK(retained_control_id(client_ui, "PrimarySlot") != 0);
  CHECK(retained_control_id(client_ui, "GearSlot") != 0);

  ::ui::Point gear_tab = retained_center_of(client_ui, "GearTab");
  run_pipeline_frame(pipeline, providers, pointer_press(), gear_tab);
  run_pipeline_frame(pipeline, providers, pointer_release(), gear_tab);
  CHECK(game.selected_weapon() == 3);

  run_pipeline_frame(pipeline, providers);
  CHECK(retained_control_id(client_ui, shooter::WEAPON_TILE_CONTROL_ID, 3) !=
        0);

  ::ui::Point primary_slot = retained_center_of(client_ui, "PrimarySlot");
  run_pipeline_frame(pipeline, providers, pointer_press(), primary_slot);
  run_pipeline_frame(pipeline, providers, pointer_release(), primary_slot);
  CHECK(game.selected_weapon() == 0);

  ::ui::Point gear_slot = retained_center_of(client_ui, "GearSlot");
  run_pipeline_frame(pipeline, providers, pointer_press(), gear_slot);
  run_pipeline_frame(pipeline, providers, pointer_release(), gear_slot);
  CHECK(game.selected_weapon() == 3);
  return true;
}

static bool loadout_from_pause_stack_commits_without_runtime_errors(void) {
  react_init_runtime();
  shooter::ShooterGame game;
  TestFrameProviders providers{.game = &game};
  client::ui::UiPipeline pipeline;
  client::ui::ClientUi &client_ui = pipeline.client_ui();
  CHECK(client_ui.push_screen(std::make_unique<shooter::ShooterGameScreen>()));

  run_pipeline_frame(pipeline, providers);
  run_pipeline_frame(pipeline, providers, keyboard_confirm());
  CHECK(client_ui.screens().count() == 2);
  CHECK(strcmp(client_ui.screens().top()->debug_name(), "Pause") == 0);

  run_pipeline_frame(pipeline, providers);
  run_pipeline_frame(pipeline, providers, keyboard_down());
  run_pipeline_frame(pipeline, providers, keyboard_down());
  run_pipeline_frame(pipeline, providers, keyboard_confirm());
  CHECK(client_ui.screens().count() == 3);
  CHECK(strcmp(client_ui.screens().top()->debug_name(), "Loadout") == 0);

  run_pipeline_frame(pipeline, providers);
  CHECK(react_error_count() == 0);
  CHECK(retained_control_id(client_ui, "WeaponsTab") != 0);
  return true;
}

static bool shooter_game_mutations_wait_for_client_ui_drain(void) {
  react_init_runtime();
  shooter::ShooterGame game;
  TestFrameProviders providers{.game = &game};
  client::ui::UiPipeline pipeline;
  client::ui::ClientUi &client_ui = pipeline.client_ui();
  CHECK(client_ui.push_screen(std::make_unique<shooter::LoadoutScreen>()));

  run_pipeline_frame(pipeline, providers);
  ::ui::Point gear_tab = retained_center_of(client_ui, "GearTab");
  run_pipeline_frame(pipeline, providers, pointer_press(), gear_tab);
  int pending_at_render = -1;
  int selected_at_render = -1;
  run_pipeline_frame(pipeline, providers, pointer_release(), gear_tab, [&] {
    pending_at_render = client_ui.pending_mutation_count();
    selected_at_render = game.selected_weapon();
  });

  // The gear tab's on_select queues two writes: one for the UI selection
  // (via the LoadoutProvider's setter) and one for the game-side
  // select_weapon. Both wait for drain.
  CHECK(selected_at_render == 0);
  CHECK(pending_at_render == 2);
  CHECK(game.selected_weapon() == 3);
  return true;
}

namespace {

struct GameAndQuitConsumerProps {
  shooter::ShooterGame **observed_game = nullptr;
  bool *observed_quit_present = nullptr;
};

static const char *screen_entry_key(const char *prefix,
                                    client::ui::UiScreenEntryId entry_id) {
  char key[64] = {};
  snprintf(key, sizeof(key), "%s-%u", prefix, entry_id);
  return ::ui::copy_string(key);
}

static ::ui::UiElement
GameAndQuitConsumerView(const GameAndQuitConsumerProps &props) {
  if (props.observed_game) {
    *props.observed_game = shooter::use_shooter_game();
  }
  if (props.observed_quit_present) {
    std::function<void()> quit = client::ui::use_request_quit();
    *props.observed_quit_present = static_cast<bool>(quit);
  }
  return ::ui::empty();
}

class GameAndQuitConsumerScreen final : public client::ui::UiScreen {
public:
  GameAndQuitConsumerScreen(shooter::ShooterGame **observed_game,
                            bool *observed_quit_present)
      : observed_game_(observed_game),
        observed_quit_present_(observed_quit_present) {}

  const char *debug_name() const override { return "GameAndQuitConsumer"; }

  bool build_element(::ui::UiElementFrame &frame,
                     ::ui::UiElement *out) override {
    if (!out)
      return false;
    *out = ::ui::component("GameAndQuitConsumerView",
                           GameAndQuitConsumerProps{
                               .observed_game = observed_game_,
                               .observed_quit_present = observed_quit_present_,
                           },
                           GameAndQuitConsumerView,
                           screen_entry_key("game-quit-consumer", entry_id()));
    return true;
  }

  void build_ui() override {}

private:
  shooter::ShooterGame **observed_game_;
  bool *observed_quit_present_;
};

// Reads use_theme() inside the retained frame and reports the resolved Button
// base gradient stop_count, so a test can prove the installed ThemeProvider
// actually delivers app_theme() (slate) rather than the neutral fallback.
struct ThemeReadProbeProps {
  const char *key = nullptr;
  int *observed_button_gradient_stops = nullptr;
};

static ::ui::UiElement ThemeReadProbeView(const ThemeReadProbeProps &props) {
  if (props.observed_button_gradient_stops) {
    *props.observed_button_gradient_stops =
        ::ui::use_theme().button.base.gradient.stop_count;
  }
  return ::ui::empty();
}

class ThemeReadProbeScreen final : public client::ui::UiScreen {
public:
  explicit ThemeReadProbeScreen(int *observed_button_gradient_stops)
      : observed_button_gradient_stops_(observed_button_gradient_stops) {}

  const char *debug_name() const override { return "ThemeReadProbe"; }

  bool build_element(::ui::UiElementFrame &frame,
                     ::ui::UiElement *out) override {
    (void)frame;
    if (!out)
      return false;
    *out = ::ui::component(
        "ThemeReadProbeView",
        ThemeReadProbeProps{.observed_button_gradient_stops =
                                observed_button_gradient_stops_},
        ThemeReadProbeView,
        screen_entry_key("theme-read-probe", entry_id()));
    return true;
  }

  void build_ui() override {}

private:
  int *observed_button_gradient_stops_;
};

} // namespace

static bool root_level_providers_reach_screens_without_per_screen_wrap(void) {
  react_init_runtime();
  shooter::ShooterGame game;
  bool quit_invoked = false;
  TestFrameProviders providers{
      .game = &game,
      .request_quit = [&quit_invoked] { quit_invoked = true; },
  };

  shooter::ShooterGame *observed_game = nullptr;
  bool observed_quit = false;

  client::ui::UiPipeline pipeline;
  CHECK(pipeline.client_ui().push_screen(
      std::make_unique<GameAndQuitConsumerScreen>(&observed_game,
                                                  &observed_quit)));

  run_pipeline_frame(pipeline, providers);
  CHECK(observed_game == &game);
  CHECK(observed_quit);
  CHECK(!quit_invoked);
  return true;
}

static bool test_theme_ownership(void) {
  // The PRODUCT look (slate, control gradients) lives in client/ via
  // app_theme(); ui/'s default_theme() is the NEUTRAL, FLAT fallback. base is a
  // dense VisualStyle, so gradient presence is the value cue stop_count != 0.
  CHECK(client::ui::app_theme().button.base.gradient.stop_count != 0); // slate has a gradient
  CHECK(::ui::default_theme().button.base.gradient.stop_count == 0);   // neutral is flat
  CHECK(client::ui::app_theme().button.focus_visible.outline.set);
  CHECK(::ui::default_theme().button.focus_visible.outline.set);
  return true;
}

static bool test_app_button_variants_distinct(void) {
  // The cva variant table must paint each non-default variant distinctly, and
  // leave Secondary as the empty patch (== the theme's default slate button).
  CHECK(shooter::app_button_variant_patch(shooter::AppButtonVariant::Danger)
            .background.set);
  CHECK(shooter::app_button_variant_patch(shooter::AppButtonVariant::Primary)
            .background.set);
  CHECK(shooter::app_button_variant_patch(shooter::AppButtonVariant::Primary)
            .background.value !=
        shooter::app_button_variant_patch(shooter::AppButtonVariant::Danger)
            .background.value);
  CHECK(shooter::app_button_variant_patch(shooter::AppButtonVariant::Ghost)
                .background.set &&
        shooter::app_button_variant_patch(shooter::AppButtonVariant::Ghost)
                .background.value.a == 0);
  // Secondary is the theme default (empty patch):
  CHECK(!shooter::app_button_variant_patch(shooter::AppButtonVariant::Secondary)
             .background.set);

  // Resolved through the slate Button base, the variants must still differ and
  // Secondary must collapse to the bare base (proves the patch wins over base).
  const ::ui::InteractionState rest{};
  const ::ui::RoleStyle &btn = client::ui::app_theme().button;
  const ::ui::VisualStyle primary = ::ui::resolve(
      btn, shooter::app_button_variant_patch(shooter::AppButtonVariant::Primary),
      rest);
  const ::ui::VisualStyle danger = ::ui::resolve(
      btn, shooter::app_button_variant_patch(shooter::AppButtonVariant::Danger),
      rest);
  const ::ui::VisualStyle secondary = ::ui::resolve(
      btn,
      shooter::app_button_variant_patch(shooter::AppButtonVariant::Secondary),
      rest);
  CHECK(primary.background != danger.background);
  CHECK(secondary.background == btn.base.background);
  return true;
}

static bool test_theme_provider_delivers_slate(void) {
  // End-to-end: a component reading use_theme() UNDER the installed
  // ThemeProvider (run_pipeline_frame wraps the tree in it, exactly like
  // production app.cpp) must see app_theme() (slate, gradient stop_count != 0),
  // NOT the provider-less neutral fallback (0). Guards against the provider
  // silently failing to push the theme context.
  react_init_runtime();
  shooter::ShooterGame game;
  TestFrameProviders providers{.game = &game, .request_quit = [] {}};
  int observed_stops = -1;
  client::ui::UiPipeline pipeline;
  CHECK(pipeline.client_ui().push_screen(
      std::make_unique<ThemeReadProbeScreen>(&observed_stops)));
  run_pipeline_frame(pipeline, providers);
  CHECK(observed_stops ==
        client::ui::app_theme().button.base.gradient.stop_count);
  CHECK(observed_stops != 0);
  return true;
}

int main(void) {
  if (!test_theme_ownership())
    return 1;
  if (!test_app_button_variants_distinct())
    return 1;
  if (!test_theme_provider_delivers_slate())
    return 1;
  if (!shooter_game_buy_and_equip_are_real_state_writes())
    return 1;
  if (!main_menu_start_match_resets_game_and_stack())
    return 1;
  if (!main_menu_options_returns_to_menu_with_cancel())
    return 1;
  if (!main_menu_quit_callback_runs())
    return 1;
  if (!shooter_screen_pushes_pause_after_confirm())
    return 1;
  if (!pause_exit_to_main_menu_resets_stack())
    return 1;
  if (!pause_options_returns_to_pause_through_screen_stack())
    return 1;
  if (!loadout_buy_uses_confirm_dialog_and_restores_parent_focus())
    return 1;
  if (!loadout_tabs_and_equipment_slots_are_real_focus_targets())
    return 1;
  if (!loadout_from_pause_stack_commits_without_runtime_errors())
    return 1;
  if (!shooter_game_mutations_wait_for_client_ui_drain())
    return 1;
  if (!root_level_providers_reach_screens_without_per_screen_wrap())
    return 1;

  react_shutdown();
  return 0;
}
