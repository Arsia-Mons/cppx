#include "client/ui/screens/lobby/components/CharacterList.h"

#include "client/commands/ClientCommand.h"
#include "ui/design/Theme.h"
#include "ui/primitives/ListItem.h"
#include "ui/primitives/Text.h"

namespace client::ui::screens::lobby {

void renderCharacterList(::ui::UiFrameContext &frame, const LobbyView &view, ClientCommandQueue &queue) {
    ::ui::Text(frame, "Characters", {.color = ::ui::color::TextMuted, .size = ::ui::type::Label, .font = ::ui::font::Body});
    for (int i = 0; i < static_cast<int>(view.characters.size()); ++i) {
        ::ui::ListItem(frame, CLAY_IDI("CharacterRow", i), view.characters[static_cast<size_t>(i)], view.selectedCharacter == i, [&, i] {
            queue.push({.type = ClientCommandType::SelectCharacter, .index = i});
        });
    }
}

}
