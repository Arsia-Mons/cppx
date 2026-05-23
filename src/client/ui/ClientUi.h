#pragma once

#include "client/ClientState.h"
#include "client/commands/ClientCommandQueue.h"
#include "ui/runtime/UiFrameContext.h"

namespace client::ui {

void render(::ui::UiFrameContext &frame, const ClientState &state, ClientCommandQueue &queue);

}
