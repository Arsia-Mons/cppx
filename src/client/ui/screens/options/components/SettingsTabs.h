#pragma once

#include "client/commands/ClientCommandQueue.h"
#include "client/ui/screens/options/OptionsState.h"
#include "ui/runtime/UiFrameContext.h"

namespace client::ui::screens::options {

void renderSettingsTabs(::ui::UiFrameContext &frame, OptionsTab activeTab, ClientCommandQueue &queue);

}
