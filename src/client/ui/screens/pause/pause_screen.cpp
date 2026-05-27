#include "pause_screen.h"

#include <memory>

#include <clay.h>

#include "../../../../react.h"
#include "../../../../ui/focus/ui_focus.h"
#include "../../../../ui/primitives/button.h"
#include "../../../../ui/primitives/clay_text.h"
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
    REACT_COMPONENT_BEGIN("PauseScreenView") {
        client::ui::ScreenNavigator nav = client::ui::use_screen_navigator();
        std::function<void()> open_options = use_push_options_screen();
        std::function<void()> open_loadout = use_push_loadout_screen();
        std::function<void()> exit_to_main_menu = use_exit_to_main_menu();
        ::ui::ui_focus_push_scope({ .id = CLAY_ID("PauseScope"), .modal = true });
        ::ui::ui_focus_request_initial_focus(CLAY_ID("ResumeButton"));

        CLAY({
            .id = CLAY_ID("PauseRoot"),
            .layout = {
                .sizing = { CLAY_SIZING_FIXED(220), CLAY_SIZING_FIT(0) },
                .padding = CLAY_PADDING_ALL(18),
                .childGap = 12,
                .layoutDirection = CLAY_TOP_TO_BOTTOM,
            },
            .backgroundColor = { 17, 24, 30, 255 },
            .cornerRadius = CLAY_CORNER_RADIUS(4),
            .border = {
                .color = { 78, 96, 108, 255 },
                .width = CLAY_BORDER_OUTSIDE(1),
            },
        }) {
            CLAY_TEXT(::ui::clay_text("Paused"),
                      CLAY_TEXT_CONFIG(
                          { .textColor = { 236, 246, 242, 255 }, .fontSize = 28 }));
            ::ui::Button({
                .id = CLAY_ID("ResumeButton"),
                .label = "Resume",
                .on_confirm = nav.pop_current,
            });
            ::ui::Button({
                .id = CLAY_ID("OpenOptionsFromPauseButton"),
                .label = "Options",
                .on_confirm = open_options,
            });
            ::ui::Button({
                .id = CLAY_ID("OpenLoadoutFromPauseButton"),
                .label = "Loadout",
                .on_confirm = open_loadout,
            });
            ::ui::Button({
                .id = CLAY_ID("ExitToMainMenuButton"),
                .label = "Exit To Menu",
                .on_confirm = exit_to_main_menu,
            });
        }

        ::ui::ui_focus_pop_scope();
    }
    REACT_COMPONENT_END();
}

void PauseScreen::build_ui() {
    REACT_COMPONENT_BEGIN_KEY("PauseScreen", entry_id()) {
        PauseScreenView();
    }
    REACT_COMPONENT_END();
}

} // namespace shooter
