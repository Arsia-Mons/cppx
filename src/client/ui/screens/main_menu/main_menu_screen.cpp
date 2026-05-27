#include "main_menu_screen.h"

#include <memory>

#include <clay.h>

#include "../in_game/in_game_screen.h"
#include "../options/options_screen.h"
#include "../../client_ui.h"
#include "../../providers/shooter_provider.h"
#include "../../../../react.h"
#include "../../../../ui/focus/ui_focus.h"
#include "../../../../ui/primitives/button.h"
#include "../../../../ui/primitives/clay_text.h"

namespace shooter {

std::function<void()> use_exit_to_main_menu() {
    ShooterGame *game = use_shooter_game();
    std::function<void()> request_quit = use_request_quit();
    client::ui::ScreenNavigator nav = client::ui::use_screen_navigator();
    return [game, request_quit, nav] {
        if (nav.reset_to && game) nav.reset_to(std::make_unique<MainMenuScreen>(game, request_quit));
    };
}

static void MainMenuScreenView() {
    REACT_COMPONENT_BEGIN("MainMenuScreenView") {
        std::function<void()> start_match  = use_start_match();
        std::function<void()> open_options = use_push_options_screen();
        std::function<void()> request_quit = use_request_quit();
        ::ui::ui_focus_push_scope({ .id = CLAY_ID("MainMenuScope") });
        ::ui::ui_focus_request_initial_focus(CLAY_ID("StartMatchButton"));

        CLAY({
            .id = CLAY_ID("MainMenuRoot"),
            .layout = {
                .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) },
                .padding = CLAY_PADDING_ALL(36),
                .childGap = 24,
                .childAlignment = { CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER },
                .layoutDirection = CLAY_TOP_TO_BOTTOM,
            },
            .backgroundColor = { 8, 14, 18, 255 },
        }) {
            CLAY({
                .id = CLAY_ID("MainMenuPanel"),
                .layout = {
                    .sizing = { CLAY_SIZING_FIXED(340), CLAY_SIZING_FIT(0) },
                    .padding = CLAY_PADDING_ALL(24),
                    .childGap = 14,
                    .layoutDirection = CLAY_TOP_TO_BOTTOM,
                },
                .backgroundColor = { 18, 27, 32, 245 },
                .cornerRadius = CLAY_CORNER_RADIUS(6),
                .border = {
                    .color = { 83, 108, 118, 255 },
                    .width = CLAY_BORDER_OUTSIDE(1),
                },
            }) {
                CLAY_TEXT(::ui::clay_text("Reference Shooter"),
                    CLAY_TEXT_CONFIG({ .textColor = { 235, 246, 242, 255 }, .fontSize = 30 }));
                CLAY_TEXT(::ui::clay_text("SDL3 / Clay UI flow"),
                    CLAY_TEXT_CONFIG({ .textColor = { 154, 177, 184, 255 }, .fontSize = 16 }));
                ::ui::Button({
                    .id = CLAY_ID("StartMatchButton"),
                    .label = "Start Match",
                    .on_confirm = start_match,
                });
                ::ui::Button({
                    .id = CLAY_ID("OpenOptionsFromMainMenuButton"),
                    .label = "Options",
                    .on_confirm = open_options,
                });
                ::ui::Button({
                    .id = CLAY_ID("QuitButton"),
                    .label = "Quit",
                    .disabled = !request_quit,
                    .on_confirm = request_quit,
                });
            }
        }

        ::ui::ui_focus_pop_scope();
    } REACT_COMPONENT_END();
}

void MainMenuScreen::build_ui() {
    ShooterProvider(game_, request_quit_, [this] {
        REACT_COMPONENT_BEGIN_KEY("MainMenuScreen", entry_id()) {
            MainMenuScreenView();
        } REACT_COMPONENT_END();
    });
}

} // namespace shooter
