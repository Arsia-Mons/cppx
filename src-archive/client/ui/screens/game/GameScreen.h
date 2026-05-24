#pragma once

#include "client/commands/ClientCommandQueue.h"
#include "client/ui/ClientUiView.h"
#include "ui/runtime/UiFrameContext.h"

namespace client::ui {

void renderGame(::ui::UiFrameContext &frame, const GameView &view, ClientCommandQueue &queue);

}
