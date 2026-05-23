#include "client/ui/screens/game/GameScreen.h"

#include "client/commands/ClientCommand.h"
#include "client/ui/screens/ScreenLayout.h"
#include "ui/layout/Box.h"
#include "ui/primitives/Button.h"
#include "ui/primitives/Text.h"

namespace client::ui {

void renderGame(::ui::UiFrameContext &frame, const GameView &view, ClientCommandQueue &queue) {
    screens::RootShell(frame, CLAY_ID("GameRoot"), [&] {
        ::ui::Box(frame, {
            .id = CLAY_ID("GameViewport"),
            .layout = {
                .sizing = ::ui::grow(),
                .padding = CLAY_PADDING_ALL(24),
                .childAlignment = {.x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER},
                .layoutDirection = CLAY_TOP_TO_BOTTOM
            },
            .backgroundColor = {15, 22, 19, 255},
            .cornerRadius = ::ui::radius::Medium,
            .border = {.color = ::ui::color::Border, .width = CLAY_BORDER_ALL(1)}
        }, [&] {
            ::ui::Heading(frame, "Hello World");
            ::ui::Text(frame, "Game screen entered as " + view.character + " on " + view.server + ".", {.color = ::ui::color::TextMuted, .size = 17, .font = ::ui::font::Body});
            ::ui::Button(frame, CLAY_ID("ExitGame"), "Exit to Menu", ::ui::ButtonVariant::Primary, [&] {
                queue.push({.type = ClientCommandType::BackToMenu});
            });
        });
    });
}

}
