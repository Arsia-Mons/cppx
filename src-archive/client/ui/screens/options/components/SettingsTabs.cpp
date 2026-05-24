#include "client/ui/screens/options/components/SettingsTabs.h"

#include "client/commands/ClientCommand.h"
#include "ui/primitives/Tabs.h"

namespace client::ui::screens::options {

void renderSettingsTabs(::ui::UiFrameContext &frame, OptionsTab activeTab, ClientCommandQueue &queue) {
    ::ui::Tab(frame, CLAY_ID("ControlsTab"), "Controls", activeTab == OptionsTab::Controls, [&] {
        queue.push({.type = ClientCommandType::SetOptionsTab, .tab = OptionsTab::Controls});
    });
    ::ui::Tab(frame, CLAY_ID("AudioTab"), "Audio", activeTab == OptionsTab::Audio, [&] {
        queue.push({.type = ClientCommandType::SetOptionsTab, .tab = OptionsTab::Audio});
    });
    ::ui::Tab(frame, CLAY_ID("VideoTab"), "Video", activeTab == OptionsTab::Video, [&] {
        queue.push({.type = ClientCommandType::SetOptionsTab, .tab = OptionsTab::Video});
    });
}

}
