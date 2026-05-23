#include "client/ui/screens/connect/ConnectScreen.h"

#include "client/commands/ClientCommand.h"
#include "client/ui/screens/ScreenLayout.h"
#include "ui/layout/Box.h"
#include "ui/primitives/Button.h"
#include "ui/primitives/Field.h"
#include "ui/primitives/Panel.h"
#include "ui/primitives/Text.h"

namespace client::ui {

void renderConnect(::ui::UiFrameContext &frame, const ConnectView &view, ClientCommandQueue &queue) {
    screens::RootShell(frame, CLAY_ID("ConnectRoot"), [&] {
        ::ui::Box(frame, {
            .id = CLAY_ID("ConnectCenter"),
            .layout = {
                .sizing = ::ui::grow(),
                .childAlignment = {.x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER}
            }
        }, [&] {
            ::ui::Panel(frame, CLAY_ID("ConnectPanel"), {.width = CLAY_SIZING_FIXED(520), .height = CLAY_SIZING_FIT(0)}, [&] {
                ::ui::Heading(frame, "Connect");
                ::ui::Text(frame, "Enter credentials for the sample connection flow.", {.color = ::ui::color::TextMuted, .size = ::ui::type::BodySmall, .font = ::ui::font::Body});
                ::ui::Field(frame, CLAY_ID("UsernameField"), "User", view.username, view.focusedField == ClientTextField::Username, false, [&] {
                    queue.push({.type = ClientCommandType::SetFocusedField, .field = ClientTextField::Username});
                });
                ::ui::Field(frame, CLAY_ID("PasswordField"), "Password", view.password, view.focusedField == ClientTextField::Password, true, [&] {
                    queue.push({.type = ClientCommandType::SetFocusedField, .field = ClientTextField::Password});
                });
                ::ui::Button(frame, CLAY_ID("ConnectSubmit"), "Connect to Lobby", ::ui::ButtonVariant::Primary, [&] {
                    queue.push({.type = ClientCommandType::Connect});
                });
                ::ui::Button(frame, CLAY_ID("ConnectBack"), "Back", ::ui::ButtonVariant::Ghost, [&] {
                    queue.push({.type = ClientCommandType::BackToMenu});
                });
            }, 22, 14);
        });
    });
}

}
