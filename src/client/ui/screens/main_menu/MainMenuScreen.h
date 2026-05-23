#pragma once

#include "client/commands/ClientCommandQueue.h"
#include "client/ui/ClientUiView.h"
#include "ui/runtime/UiFrameContext.h"

namespace client::ui {

void renderMainMenu(::ui::UiFrameContext &frame, const MainMenuView &view, ClientCommandQueue &queue);

}
