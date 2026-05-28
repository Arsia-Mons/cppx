#include "client/ui/client_ui.h"
#include "client/ui/ui_pipeline.h"
#include "react.h"
#include "game/shooter_game.h"
#include "client/ui/providers/app_shell.h"
#include "client/ui/providers/shooter_provider.h"
#include "client/ui/screens/in_game/in_game_screen.h"
#include "client/ui/screens/loadout/components/weapon_tile.h"
#include "client/ui/screens/loadout/loadout_screen.h"
#include "client/ui/screens/main_menu/main_menu_screen.h"
#include "client/ui/screens/options/options_screen.h"
#include "client/ui/screens/pause/pause_screen.h"

#include <functional>

#include <memory>
#include <stdio.h>
#include <string.h>

#define CHECK(expr)                                                              \
    do {                                                                         \
        if (!(expr)) {                                                           \
            fprintf(stderr, "CHECK failed at %s:%d: %s\n", __FILE__, __LINE__,  \
                    #expr);                                                      \
            return false;                                                        \
        }                                                                        \
    } while (0)

struct TestFrameProviders {
    shooter::ShooterGame                *game         = nullptr;
    std::function<void()>                request_quit = {};
};

static void run_pipeline_frame(client::ui::UiPipeline &pipeline,
                               const TestFrameProviders &providers,
                               const ::ui::UiInputFrame &input = {},
                               ::ui::retained::Point pointer = { -1000.0f, -1000.0f },
                               const client::ui::RenderFrame &render = {}) {
    pipeline.set_frame_provider([&](const std::function<void()> &build) {
        shooter::ShooterContextValue     game_ctx { .game = providers.game };
        client::ui::AppShellContextValue shell_ctx { .request_quit = providers.request_quit };
        shooter::shooter_provider_push(&game_ctx);
        client::ui::app_shell_provider_push(&shell_ctx);
        build();
        client::ui::app_shell_provider_pop();
        shooter::shooter_provider_pop();
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

static ::ui::retained::NodeId find_retained_control(
    const ::ui::retained::UiTree &tree,
    ::ui::retained::NodeId id,
    const char *name,
    int offset = 0) {
    ::ui::retained::NodeSnapshot node = {};
    if (!tree.snapshot(id, &node))
        return 0;
    if (strcmp(node.control_id ? node.control_id : "", name) == 0 &&
        node.control_offset == offset) {
        return id;
    }
    for (int i = 0; i < tree.child_count(id); ++i) {
        ::ui::retained::NodeId found =
            find_retained_control(tree, tree.child_at(id, i), name, offset);
        if (found != 0)
            return found;
    }
    return 0;
}

static ::ui::retained::NodeId retained_control_id(
    const client::ui::ClientUi &client_ui,
    const char *name,
    int offset = 0) {
    const ::ui::retained::UiTree &tree = client_ui.retained_tree();
    return find_retained_control(tree, tree.root_id(), name, offset);
}

static ::ui::retained::Point retained_center_of(
    const client::ui::ClientUi &client_ui,
    const char *name,
    int offset = 0) {
    const ::ui::retained::UiTree &tree = client_ui.retained_tree();
    ::ui::retained::NodeId id = retained_control_id(client_ui, name, offset);
    ::ui::retained::NodeSnapshot node = {};
    if (id == 0 || !tree.snapshot(id, &node))
        return { -1000.0f, -1000.0f };
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
    TestFrameProviders providers {
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
    TestFrameProviders providers { .game = &game };
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
    TestFrameProviders providers {
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
    TestFrameProviders providers { .game = &game };
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
    TestFrameProviders providers { .game = &game };
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
    TestFrameProviders providers { .game = &game };
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
    TestFrameProviders providers { .game = &game };
    client::ui::UiPipeline pipeline;
    client::ui::ClientUi &client_ui = pipeline.client_ui();
    CHECK(client_ui.push_screen(std::make_unique<shooter::LoadoutScreen>()));

    run_pipeline_frame(pipeline, providers);
    CHECK(::ui::retained::focus_focused_id(client_ui.retained_focus()) ==
          retained_control_id(client_ui, shooter::WEAPON_TILE_CONTROL_ID, 0));
    CHECK(game.selected_weapon() == 0);

    run_pipeline_frame(pipeline, providers, keyboard_right());
    CHECK(::ui::retained::focus_focused_id(client_ui.retained_focus()) ==
          retained_control_id(client_ui, shooter::WEAPON_TILE_CONTROL_ID, 1));
    // Focus is a pure-UI change: focusing tile 1 must NOT mutate game state.
    CHECK(game.selected_weapon() == 0);
    CHECK(!game.weapon(1).owned);
    CHECK(game.credits() == 450);

    run_pipeline_frame(pipeline, providers, keyboard_right());
    run_pipeline_frame(pipeline, providers, keyboard_down());
    ::ui::retained::NodeId buy_id =
        retained_control_id(client_ui, "BuyWeaponButton");
    CHECK(::ui::retained::focus_focused_id(client_ui.retained_focus()) == buy_id);

    run_pipeline_frame(pipeline, providers, keyboard_confirm());
    CHECK(!game.weapon(1).owned);
    CHECK(game.credits() == 450);

    run_pipeline_frame(pipeline, providers);
    CHECK(::ui::retained::focus_focused_id(client_ui.retained_focus()) ==
          retained_control_id(client_ui, "ConfirmLoadoutActionButton"));
    CHECK(client_ui.retained_focus().previous_focus_before_modal == buy_id);

    run_pipeline_frame(pipeline, providers, keyboard_right());
    CHECK(::ui::retained::focus_focused_id(client_ui.retained_focus()) ==
          retained_control_id(client_ui, "CancelLoadoutActionButton"));
    run_pipeline_frame(pipeline, providers, keyboard_confirm());
    CHECK(!game.weapon(1).owned);
    CHECK(game.credits() == 450);

    run_pipeline_frame(pipeline, providers);
    buy_id = retained_control_id(client_ui, "BuyWeaponButton");
    CHECK(::ui::retained::focus_focused_id(client_ui.retained_focus()) == buy_id);

    run_pipeline_frame(pipeline, providers, keyboard_confirm());
    CHECK(!game.weapon(1).owned);
    run_pipeline_frame(pipeline, providers);
    CHECK(::ui::retained::focus_focused_id(client_ui.retained_focus()) ==
          retained_control_id(client_ui, "ConfirmLoadoutActionButton"));
    run_pipeline_frame(pipeline, providers, keyboard_confirm());
    CHECK(game.weapon(1).owned);
    CHECK(game.credits() == 150);
    return true;
}

static bool loadout_tabs_and_equipment_slots_are_real_focus_targets(void) {
    react_init_runtime();
    shooter::ShooterGame game;
    TestFrameProviders providers { .game = &game };
    client::ui::UiPipeline pipeline;
    client::ui::ClientUi &client_ui = pipeline.client_ui();
    CHECK(client_ui.push_screen(std::make_unique<shooter::LoadoutScreen>()));

    run_pipeline_frame(pipeline, providers);
    CHECK(retained_control_id(client_ui, "WeaponsTab") != 0);
    CHECK(retained_control_id(client_ui, "GearTab") != 0);
    CHECK(retained_control_id(client_ui, "PrimarySlot") != 0);
    CHECK(retained_control_id(client_ui, "GearSlot") != 0);

    ::ui::retained::Point gear_tab = retained_center_of(client_ui, "GearTab");
    run_pipeline_frame(pipeline, providers, pointer_press(), gear_tab);
    run_pipeline_frame(pipeline, providers, pointer_release(), gear_tab);
    CHECK(game.selected_weapon() == 3);

    run_pipeline_frame(pipeline, providers);
    CHECK(retained_control_id(client_ui, shooter::WEAPON_TILE_CONTROL_ID, 3) != 0);

    ::ui::retained::Point primary_slot = retained_center_of(client_ui, "PrimarySlot");
    run_pipeline_frame(pipeline, providers, pointer_press(), primary_slot);
    run_pipeline_frame(pipeline, providers, pointer_release(), primary_slot);
    CHECK(game.selected_weapon() == 0);

    ::ui::retained::Point gear_slot = retained_center_of(client_ui, "GearSlot");
    run_pipeline_frame(pipeline, providers, pointer_press(), gear_slot);
    run_pipeline_frame(pipeline, providers, pointer_release(), gear_slot);
    CHECK(game.selected_weapon() == 3);
    return true;
}

static bool shooter_game_mutations_wait_for_client_ui_drain(void) {
    react_init_runtime();
    shooter::ShooterGame game;
    TestFrameProviders providers { .game = &game };
    client::ui::UiPipeline pipeline;
    client::ui::ClientUi &client_ui = pipeline.client_ui();
    CHECK(client_ui.push_screen(std::make_unique<shooter::LoadoutScreen>()));

    run_pipeline_frame(pipeline, providers);
    ::ui::retained::Point gear_tab = retained_center_of(client_ui, "GearTab");
    run_pipeline_frame(pipeline, providers, pointer_press(), gear_tab);
    int pending_at_render = -1;
    int selected_at_render = -1;
    run_pipeline_frame(
        pipeline, providers, pointer_release(), gear_tab,
        [&] {
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

static const char *screen_entry_key(::ui::retained::UiElementFrame &frame,
                                    const char *prefix,
                                    client::ui::UiScreenEntryId entry_id) {
    char key[64] = {};
    snprintf(key, sizeof(key), "%s-%u", prefix, entry_id);
    return frame.copy_string(key);
}

static ::ui::retained::UiElement
render_game_and_quit_consumer(const GameAndQuitConsumerProps &props,
                              ::ui::retained::UiElementFrame &frame) {
    if (props.observed_game) {
        *props.observed_game = shooter::use_shooter_game();
    }
    if (props.observed_quit_present) {
        std::function<void()> quit = client::ui::use_request_quit();
        *props.observed_quit_present = static_cast<bool>(quit);
    }
    return frame.empty();
}

class GameAndQuitConsumerScreen final : public client::ui::UiScreen {
public:
    GameAndQuitConsumerScreen(shooter::ShooterGame **observed_game,
                              bool *observed_quit_present)
        : observed_game_(observed_game),
          observed_quit_present_(observed_quit_present) {}

    const char *debug_name() const override { return "GameAndQuitConsumer"; }

    bool build_element(::ui::retained::UiElementFrame &frame,
                       ::ui::retained::UiElement *out) override {
        if (!out)
            return false;
        *out = frame.component(
            "GameAndQuitConsumerView",
            GameAndQuitConsumerProps{
                .observed_game = observed_game_,
                .observed_quit_present = observed_quit_present_,
            },
            render_game_and_quit_consumer,
            screen_entry_key(frame, "game-quit-consumer", entry_id()));
        return true;
    }

    void build_ui() override {}

private:
    shooter::ShooterGame **observed_game_;
    bool                  *observed_quit_present_;
};

} // namespace

static bool root_level_providers_reach_screens_without_per_screen_wrap(void) {
    react_init_runtime();
    shooter::ShooterGame game;
    bool quit_invoked = false;
    TestFrameProviders providers {
        .game = &game,
        .request_quit = [&quit_invoked] { quit_invoked = true; },
    };

    shooter::ShooterGame *observed_game     = nullptr;
    bool                  observed_quit     = false;

    client::ui::UiPipeline pipeline;
    CHECK(pipeline.client_ui().push_screen(
        std::make_unique<GameAndQuitConsumerScreen>(&observed_game, &observed_quit)));

    run_pipeline_frame(pipeline, providers);
    CHECK(observed_game == &game);
    CHECK(observed_quit);
    CHECK(!quit_invoked);
    return true;
}

int main(void) {
    if (!shooter_game_buy_and_equip_are_real_state_writes()) return 1;
    if (!main_menu_start_match_resets_game_and_stack()) return 1;
    if (!main_menu_options_returns_to_menu_with_cancel()) return 1;
    if (!main_menu_quit_callback_runs()) return 1;
    if (!shooter_screen_pushes_pause_after_confirm()) return 1;
    if (!pause_exit_to_main_menu_resets_stack()) return 1;
    if (!pause_options_returns_to_pause_through_screen_stack()) return 1;
    if (!loadout_buy_uses_confirm_dialog_and_restores_parent_focus()) return 1;
    if (!loadout_tabs_and_equipment_slots_are_real_focus_targets()) return 1;
    if (!shooter_game_mutations_wait_for_client_ui_drain()) return 1;
    if (!root_level_providers_reach_screens_without_per_screen_wrap()) return 1;

    react_shutdown();
    return 0;
}
