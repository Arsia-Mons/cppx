#include "client/ui/screens/lobby/components/ServerList.h"

#include "client/commands/ClientCommand.h"
#include "ui/design/Theme.h"
#include "ui/primitives/ListItem.h"
#include "ui/primitives/Text.h"

namespace client::ui::screens::lobby {

void renderServerList(::ui::UiFrameContext &frame, const LobbyView &view, ClientCommandQueue &queue) {
    ::ui::Text(frame, "Servers", {.color = ::ui::color::TextMuted, .size = ::ui::type::Label, .font = ::ui::font::Body});
    for (int i = 0; i < static_cast<int>(view.servers.size()); ++i) {
        ::ui::ListItem(frame, CLAY_IDI("ServerRow", i), view.servers[static_cast<size_t>(i)], view.selectedServer == i, [&, i] {
            queue.push({.type = ClientCommandType::SelectServer, .index = i});
        });
    }
}

}
