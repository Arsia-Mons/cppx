#include "client/ui/screens/main_menu/MainMenuScreen.h"

#include "client/ui/screens/ScreenLayout.h"
#include "client/ui/screens/main_menu/components/MenuPanel.h"
#include "ui/layout/Box.h"

namespace client::ui {

void renderMainMenu(::ui::UiFrameContext &frame, const MainMenuView &, ClientCommandQueue &queue) {
    screens::RootShell(frame, CLAY_ID("MainMenuRoot"), [&] {
        ::ui::Box(frame, {
            .id = CLAY_ID("MenuHero"),
            .layout = {
                .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0)},
                .childAlignment = {.x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER}
            }
        }, [&] {
            screens::main_menu::renderMenuPanel(frame, queue);
        });
    });
}

}
