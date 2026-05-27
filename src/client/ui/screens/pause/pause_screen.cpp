#include "pause_screen.h"

#include <memory>

#include "../../../../react.h"
#include "../../../../ui/retained/components.h"
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

static void PauseScreenView() {
    REACT_RETAINED_COMPONENT_BEGIN("PauseScreenView") {
        client::ui::ScreenNavigator nav = client::ui::use_screen_navigator();
        bool is_top = client::ui::use_screen_is_top();
        std::function<void()> open_options = use_push_options_screen();
        std::function<void()> open_loadout = use_push_loadout_screen();
        std::function<void()> exit_to_main_menu = use_exit_to_main_menu();
        namespace retained = ::ui::retained;

        if (is_top) {
            retained::Panel(
                {
                    .key = "root",
                    .width = retained::Length::percent(100.0f),
                    .height = retained::Length::percent(100.0f),
                    .direction = retained::FlexDirection::Column,
                    .align_items = retained::AlignItems::Center,
                    .justify_content = retained::JustifyContent::Center,
                    .modal = true,
                },
                [&] {
                    retained::Panel(
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
                        },
                        [&] {
                            retained::Text({
                                .key = "title",
                                .value = "Paused",
                                .height = retained::Length::points(34.0f),
                                .text_color = {236, 246, 242, 255},
                                .font_size = 28,
                            });
                            retained::Button({
                                .key = "resume",
                                .id = "ResumeButton",
                                .label = "Resume",
                                .on_confirm = nav.pop_current,
                            });
                            retained::Button({
                                .key = "options",
                                .id = "OpenOptionsFromPauseButton",
                                .label = "Options",
                                .on_confirm = open_options,
                            });
                            retained::Button({
                                .key = "loadout",
                                .id = "OpenLoadoutFromPauseButton",
                                .label = "Loadout",
                                .on_confirm = open_loadout,
                            });
                            retained::Button({
                                .key = "exit-to-menu",
                                .id = "ExitToMainMenuButton",
                                .label = "Exit To Menu",
                                .on_confirm = exit_to_main_menu,
                            });
                        });
                });
        }
    } REACT_RETAINED_COMPONENT_END();
}

void PauseScreen::build_ui() {
    REACT_RETAINED_COMPONENT_BEGIN_KEY("PauseScreen", entry_id()) {
        PauseScreenView();
    } REACT_RETAINED_COMPONENT_END();
}

} // namespace shooter
