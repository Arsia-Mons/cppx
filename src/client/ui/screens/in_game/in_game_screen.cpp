#include "in_game_screen.h"

#include <memory>

#include "../../../../react.h"
#include "../../../../ui/retained/components.h"
#include "../../callback_deps.h"
#include "../../client_ui.h"
#include "../../components/hud_band.h"
#include "../../providers/shooter_provider.h"
#include "../../internal/deferred_ui_mutation.h"
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
        client::ui::callback_deps(client::ui::callback_deps_ptr(game),
                                  client::ui::callback_deps_ptr(mutations.owner()),
                                  nav.current_entry_id));
}

static void ShooterGameScreenView() {
    REACT_RETAINED_COMPONENT_BEGIN("ShooterGameScreenView") {
        bool is_top = client::ui::use_screen_is_top();
        std::function<void()> open_pause = use_push_pause_screen();
        std::function<void()> open_loadout = use_push_loadout_screen();
        namespace retained = ::ui::retained;

        if (is_top) {
            retained::Panel(
                {
                    .key = "root",
                    .width = retained::Length::percent(100.0f),
                    .height = retained::Length::percent(100.0f),
                    .direction = retained::FlexDirection::Column,
                    .padding = {24.0f, 24.0f, 24.0f, 24.0f},
                    .gap = 18.0f,
                    .background = {10, 16, 18, 255},
                },
                [&] {
                    HudBand();
                    retained::Panel(
                        {
                            .key = "actions",
                            .direction = retained::FlexDirection::Row,
                            .align_items = retained::AlignItems::Start,
                            .gap = 12.0f,
                        },
                        [&] {
                            retained::Button({
                                .key = "pause",
                                .id = "OpenPauseButton",
                                .label = "Pause",
                                .on_confirm = open_pause,
                            });
                            retained::Button({
                                .key = "loadout",
                                .id = "OpenLoadoutButton",
                                .label = "Loadout",
                                .on_confirm = open_loadout,
                            });
                        });
                });
        }
    } REACT_RETAINED_COMPONENT_END();
}

void ShooterGameScreen::build_ui() {
    REACT_RETAINED_COMPONENT_BEGIN_KEY("ShooterGameScreen", entry_id()) {
        ShooterGameScreenView();
    } REACT_RETAINED_COMPONENT_END();
}

} // namespace shooter
