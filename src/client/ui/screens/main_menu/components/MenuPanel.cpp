#include "client/ui/screens/main_menu/components/MenuPanel.h"

#include "client/commands/ClientCommand.h"
#include "client/ui/navigation/ScreenId.h"
#include "ui/design/Theme.h"
#include "ui/primitives/Button.h"
#include "ui/primitives/Panel.h"
#include "ui/primitives/Text.h"

namespace client::ui::screens::main_menu {

void renderMenuPanel(::ui::UiFrameContext &frame, ClientCommandQueue &queue) {
    ::ui::Panel(frame, CLAY_ID("MenuPanel"), {.width = CLAY_SIZING_FIXED(440), .height = CLAY_SIZING_FIT(0)}, [&] {
        ::ui::Heading(frame, "Clay SDL3 Game UI");
        ::ui::Text(frame, "A complete menu-to-lobby flow built from composable Clay primitives.", {.color = ::ui::color::TextMuted, .size = 17, .font = ::ui::font::Body});
        ::ui::Button(frame, CLAY_ID("StartConnect"), "Connect", ::ui::ButtonVariant::Primary, [&] {
            queue.push({.type = ClientCommandType::Navigate, .screen = navigation::ScreenId::Connect});
        });
        ::ui::Button(frame, CLAY_ID("Options"), "Options", ::ui::ButtonVariant::Secondary, [&] {
            queue.push({.type = ClientCommandType::Navigate, .screen = navigation::ScreenId::Options});
        });
        ::ui::Button(frame, CLAY_ID("Quit"), "Quit", ::ui::ButtonVariant::Ghost, [&] {
            queue.push({.type = ClientCommandType::Quit});
        });
    }, 22, 14);
}

}
