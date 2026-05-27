#include "in_game_screen.h"

#include <memory>

#include <clay.h>

#include "../../../../react.h"
#include "../../../../ui/focus/ui_focus.h"
#include "../../../../ui/primitives/button.h"
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
    REACT_COMPONENT_BEGIN("ShooterGameScreenView") {
        std::function<void()> open_pause = use_push_pause_screen();
        std::function<void()> open_loadout = use_push_loadout_screen();
        ::ui::ui_focus_push_scope({ .id = CLAY_ID("ShooterGameScope") });
        ::ui::ui_focus_request_initial_focus(CLAY_ID("OpenPauseButton"));

        CLAY({
            .id = CLAY_ID("ShooterGameRoot"),
            .layout = {
                .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) },
                .padding = CLAY_PADDING_ALL(24),
                .childGap = 18,
                .layoutDirection = CLAY_TOP_TO_BOTTOM,
            },
            .backgroundColor = { 10, 16, 18, 255 },
        }) {
            HudBand();
            CLAY({
                .id = CLAY_ID("ShooterGameActions"),
                .layout = {
                    .sizing = { CLAY_SIZING_FIT(0), CLAY_SIZING_FIT(0) },
                    .childGap = 12,
                    .layoutDirection = CLAY_LEFT_TO_RIGHT,
                },
            }) {
                ::ui::Button({
                    .id = CLAY_ID("OpenPauseButton"),
                    .label = "Pause",
                    .on_confirm = open_pause,
                });
                ::ui::Button({
                    .id = CLAY_ID("OpenLoadoutButton"),
                    .label = "Loadout",
                    .on_confirm = open_loadout,
                });
            }
        }

        ::ui::ui_focus_pop_scope();
    }
    REACT_COMPONENT_END();
}

void ShooterGameScreen::build_ui() {
    REACT_COMPONENT_BEGIN_KEY("ShooterGameScreen", entry_id()) {
        ShooterGameScreenView();
    }
    REACT_COMPONENT_END();
}

} // namespace shooter
