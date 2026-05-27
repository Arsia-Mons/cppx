#include "main_menu_screen.h"

#include <memory>

#include "../../../../react.h"
#include "../../../../ui/retained/components.h"
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

static void MainMenuScreenView() {
    REACT_RETAINED_COMPONENT_BEGIN("MainMenuScreenView") {
        std::function<void()> start_match = use_start_match();
        std::function<void()> open_options = use_push_options_screen();
        std::function<void()> request_quit = client::ui::use_request_quit();
        namespace retained = ::ui::retained;

        retained::Panel(
            {
                .key = "root",
                .width = retained::Length::percent(100.0f),
                .height = retained::Length::percent(100.0f),
                .direction = retained::FlexDirection::Column,
                .align_items = retained::AlignItems::Center,
                .justify_content = retained::JustifyContent::Center,
                .padding = {36.0f, 36.0f, 36.0f, 36.0f},
                .background = {8, 14, 18, 255},
            },
            [&] {
                retained::Panel(
                    {
                        .key = "panel",
                        .width = retained::Length::points(340.0f),
                        .direction = retained::FlexDirection::Column,
                        .align_items = retained::AlignItems::Center,
                        .padding = {24.0f, 24.0f, 24.0f, 24.0f},
                        .gap = 14.0f,
                        .background = {18, 27, 32, 245},
                        .border = {83, 108, 118, 255},
                        .border_width = 1.0f,
                    },
                    [&] {
                        retained::Text({
                            .key = "title",
                            .value = "Reference Shooter",
                            .height = retained::Length::points(36.0f),
                            .text_color = {235, 246, 242, 255},
                            .font_size = 30,
                        });
                        retained::Text({
                            .key = "subtitle",
                            .value = "SDL3 / retained UI flow",
                            .height = retained::Length::points(22.0f),
                            .text_color = {154, 177, 184, 255},
                            .font_size = 16,
                        });
                        retained::Button({
                            .key = "start",
                            .id = "StartMatchButton",
                            .label = "Start Match",
                            .on_confirm = start_match,
                        });
                        retained::Button({
                            .key = "options",
                            .id = "OpenOptionsFromMainMenuButton",
                            .label = "Options",
                            .on_confirm = open_options,
                        });
                        retained::Button({
                            .key = "quit",
                            .id = "QuitButton",
                            .label = "Quit",
                            .disabled = !request_quit,
                            .on_confirm = request_quit,
                        });
                    });
            });
    } REACT_RETAINED_COMPONENT_END();
}

void MainMenuScreen::build_ui() {
    REACT_RETAINED_COMPONENT_BEGIN_KEY("MainMenuScreen", entry_id()) {
        MainMenuScreenView();
    } REACT_RETAINED_COMPONENT_END();
}

} // namespace shooter
