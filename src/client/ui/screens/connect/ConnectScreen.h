#pragma once

#include "client/commands/ClientCommandQueue.h"
#include "client/ui/ClientUiView.h"
#include "ui/runtime/UiFrameContext.h"

namespace client::ui {

void renderConnect(::ui::UiFrameContext &frame, const ConnectView &view, ClientCommandQueue &queue);

}
