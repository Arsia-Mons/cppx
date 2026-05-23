#pragma once

#include "client/commands/ClientCommandQueue.h"
#include "client/ui/ClientUiView.h"
#include "ui/runtime/UiFrameContext.h"

namespace client::ui {

void renderOptions(::ui::UiFrameContext &frame, const OptionsView &view, ClientCommandQueue &queue);

}
