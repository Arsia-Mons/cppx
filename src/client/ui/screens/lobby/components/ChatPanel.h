#pragma once

#include "client/commands/ClientCommandQueue.h"
#include "client/ui/ClientUiView.h"
#include "ui/runtime/UiFrameContext.h"

namespace client::ui::screens::lobby {

void renderChatPanel(::ui::UiFrameContext &frame, const LobbyView &view, ClientCommandQueue &queue);

}
