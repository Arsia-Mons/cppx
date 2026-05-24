#include "client/ui/screens/lobby/components/ChatPanel.h"

#include "client/commands/ClientCommand.h"
#include "ui/design/Theme.h"
#include "ui/layout/Box.h"
#include "ui/layout/ScrollArea.h"
#include "ui/primitives/Button.h"
#include "ui/primitives/Field.h"
#include "ui/primitives/Text.h"

namespace client::ui::screens::lobby {

void renderChatPanel(::ui::UiFrameContext &frame, const LobbyView &view, ClientCommandQueue &queue) {
    ::ui::Text(frame, "Lobby Chat", {.color = ::ui::color::TextMuted, .size = ::ui::type::Label, .font = ::ui::font::Body});
    ::ui::ScrollArea(frame, CLAY_ID("ChatScroll"), {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0)}, [&] {
        for (int i = 0; i < static_cast<int>(view.chat.size()); ++i) {
            const ChatMessage &message = view.chat[static_cast<size_t>(i)];
            ::ui::Box(frame, {
                .id = CLAY_IDI("ChatMessage", i),
                .layout = {
                    .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIT(0)},
                    .padding = CLAY_PADDING_ALL(8),
                    .childGap = 4,
                    .layoutDirection = CLAY_TOP_TO_BOTTOM
                },
                .backgroundColor = ::ui::color::Surface,
                .cornerRadius = ::ui::radius::Small
            }, [&] {
                ::ui::Text(frame, message.author, {.color = ::ui::color::Accent, .size = ::ui::type::Caption, .font = ::ui::font::Body});
                ::ui::Text(frame, message.body, {.color = ::ui::color::Text, .size = ::ui::type::BodySmall, .font = ::ui::font::Body});
            });
        }
    });
    ::ui::Field(frame, CLAY_ID("ChatDraft"), "Message", view.chatDraft, view.focusedField == ClientTextField::Chat, false, [&] {
        queue.push({.type = ClientCommandType::SetFocusedField, .field = ClientTextField::Chat});
    });
    ::ui::Button(frame, CLAY_ID("SendChat"), "Send", ::ui::ButtonVariant::Secondary, [&] {
        queue.push({.type = ClientCommandType::SendChat});
    });
}

}
