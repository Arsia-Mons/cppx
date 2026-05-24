#include "client/ui/screens/lobby/components/CreateGamePanel.h"

#include "client/commands/ClientCommand.h"
#include "ui/design/Theme.h"
#include "ui/primitives/Button.h"
#include "ui/primitives/Field.h"
#include "ui/primitives/Text.h"

namespace client::ui::screens::lobby {

void renderCreateGamePanel(::ui::UiFrameContext &frame, const LobbyView &view, ClientCommandQueue &queue) {
    ::ui::Text(frame, "Create Game", {.color = ::ui::color::TextMuted, .size = ::ui::type::Label, .font = ::ui::font::Body});
    ::ui::Field(frame, CLAY_ID("GameNameField"), "Game name", view.gameName, view.focusedField == ClientTextField::GameName, false, [&] {
        queue.push({.type = ClientCommandType::SetFocusedField, .field = ClientTextField::GameName});
    });
    ::ui::Text(frame, "Server: " + view.servers[static_cast<size_t>(view.selectedServer)], {.color = ::ui::color::Text, .size = ::ui::type::BodySmall, .font = ::ui::font::Body});
    ::ui::Text(frame, "Character: " + view.characters[static_cast<size_t>(view.selectedCharacter)], {.color = ::ui::color::Text, .size = ::ui::type::BodySmall, .font = ::ui::font::Body});
    ::ui::Button(frame, CLAY_ID("CreateGame"), "Create", ::ui::ButtonVariant::Secondary, [&] {
        queue.push({.type = ClientCommandType::CreateGame});
    });
}

}
