#include "options_screen.h"

#include <memory>

#include "../../../../react.h"
#include "../../../../ui/retained/components.h"
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

static void OptionsScreenView() {
    REACT_RETAINED_COMPONENT_BEGIN("OptionsScreenView") {
        client::ui::ScreenNavigator nav = client::ui::use_screen_navigator();
        bool is_top = client::ui::use_screen_is_top();
        int *large_hud = use_state_int(0);
        int *reduced_motion = use_state_int(1);
        namespace retained = ::ui::retained;

        if (is_top) {
            retained::Panel(
                {
                    .key = "root",
                    .width = retained::Length::percent(100.0f),
                    .height = retained::Length::percent(100.0f),
                    .direction = retained::FlexDirection::Column,
                    .align_items = retained::AlignItems::Start,
                    .padding = {24.0f, 24.0f, 24.0f, 24.0f},
                    .gap = 12.0f,
                    .modal = true,
                    .background = {14, 22, 28, 245},
                },
                [&] {
                    retained::Text({
                        .key = "title",
                        .value = "Options",
                        .height = retained::Length::points(32.0f),
                        .text_color = {236, 246, 242, 255},
                        .font_size = 26,
                    });
                    retained::Toggle({
                        .key = "large-hud",
                        .id = "LargeHudToggle",
                        .label = "Large HUD",
                        .checked = *large_hud != 0,
                        .on_change = [large_hud](bool enabled) {
                            *large_hud = enabled ? 1 : 0;
                        },
                    });
                    retained::Toggle({
                        .key = "reduced-motion",
                        .id = "ReducedMotionToggle",
                        .label = "Reduce Motion",
                        .checked = *reduced_motion != 0,
                        .on_change = [reduced_motion](bool enabled) {
                            *reduced_motion = enabled ? 1 : 0;
                        },
                    });
                    retained::Button({
                        .key = "back",
                        .id = "BackFromOptionsButton",
                        .label = "Back",
                        .on_confirm = nav.pop_current,
                    });
                });
        }
    } REACT_RETAINED_COMPONENT_END();
}

void OptionsScreen::build_ui() {
    REACT_RETAINED_COMPONENT_BEGIN_KEY("OptionsScreen", entry_id()) {
        OptionsScreenView();
    } REACT_RETAINED_COMPONENT_END();
}

} // namespace shooter
