#include "demo_app_screen.h"

#include "../../react.h"
#include "../../ui/components/app.h"
#include "game_ui_pipeline.h"

namespace game::ui {

void DemoAppScreen::build_ui() {
    REACT_COMPONENT_BEGIN_KEY("DemoAppScreenView", entry_id()) {
        App(use_demo_app_input());
    } REACT_COMPONENT_END();
}

} // namespace game::ui
