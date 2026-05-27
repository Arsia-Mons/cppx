#include "options_screen.h"

#include <memory>

#include <clay.h>

#include "../../client_ui.h"
#include "../../../../react.h"
#include "../../../../ui/focus/ui_focus.h"
#include "../../../../ui/primitives/button.h"
#include "../../../../ui/primitives/clay_text.h"
#include "../../../../ui/primitives/toggle.h"

namespace shooter {

std::function<void()> use_push_options_screen() {
    client::ui::ScreenNavigator nav = client::ui::use_screen_navigator();
    return [nav] {
        if (nav.push) nav.push(std::make_unique<OptionsScreen>());
    };
}

static void OptionsScreenView() {
    REACT_COMPONENT_BEGIN("OptionsScreenView") {
        client::ui::ScreenNavigator nav = client::ui::use_screen_navigator();
        int *large_hud      = use_state_int(0);
        int *reduced_motion = use_state_int(1);
        ::ui::ui_focus_push_scope({ .id = CLAY_ID("OptionsScope"), .modal = true });
        ::ui::ui_focus_request_initial_focus(CLAY_ID("LargeHudToggle"));

        CLAY({
            .id = CLAY_ID("OptionsRoot"),
            .layout = {
                .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) },
                .padding = CLAY_PADDING_ALL(24),
                .childGap = 12,
                .layoutDirection = CLAY_TOP_TO_BOTTOM,
            },
            .backgroundColor = { 14, 22, 28, 245 },
        }) {
            CLAY_TEXT(::ui::clay_text("Options"),
                CLAY_TEXT_CONFIG({ .textColor = { 236, 246, 242, 255 }, .fontSize = 26 }));
            ::ui::Toggle({
                .id = CLAY_ID("LargeHudToggle"),
                .label = "Large HUD",
                .checked = *large_hud != 0,
                .on_change = [large_hud](bool enabled) { *large_hud = enabled ? 1 : 0; },
            });
            ::ui::Toggle({
                .id = CLAY_ID("ReducedMotionToggle"),
                .label = "Reduce Motion",
                .checked = *reduced_motion != 0,
                .on_change = [reduced_motion](bool enabled) { *reduced_motion = enabled ? 1 : 0; },
            });
            ::ui::Button({
                .id = CLAY_ID("BackFromOptionsButton"),
                .label = "Back",
                .on_confirm = nav.pop_current,
            });
        }

        ::ui::ui_focus_pop_scope();
    } REACT_COMPONENT_END();
}

void OptionsScreen::build_ui() {
    REACT_COMPONENT_BEGIN_KEY("OptionsScreen", entry_id()) {
        OptionsScreenView();
    } REACT_COMPONENT_END();
}

} // namespace shooter
