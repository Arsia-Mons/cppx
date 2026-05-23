#pragma once

#include "client/commands/ClientCommandQueue.h"
#include "ui/runtime/UiFrameContext.h"

namespace client::ui::screens::main_menu {

void renderMenuPanel(::ui::UiFrameContext &frame, ClientCommandQueue &queue);

}
