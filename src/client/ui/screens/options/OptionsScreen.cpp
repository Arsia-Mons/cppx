#include "client/ui/screens/options/OptionsScreen.h"

#include "client/commands/ClientCommand.h"
#include "client/ui/screens/ScreenLayout.h"
#include "client/ui/screens/options/components/SettingsTabs.h"
#include "ui/layout/Box.h"
#include "ui/primitives/Button.h"
#include "ui/primitives/Panel.h"
#include "ui/primitives/ProgressBar.h"
#include "ui/primitives/Text.h"

namespace client::ui {
namespace {

using OptionsTab = screens::options::OptionsTab;

void renderControls(::ui::UiFrameContext &frame) {
    ::ui::Text(frame, "Keyboard", {.color = ::ui::color::TextMuted, .size = ::ui::type::Label, .font = ::ui::font::Body});
    ::ui::Text(frame, "WASD / Arrow Keys: Navigate game world");
    ::ui::Text(frame, "Enter: Confirm focused action");
    ::ui::Text(frame, "Escape: Back or menu");
    ::ui::Text(frame, "Mouse wheel: Scroll Clay containers");
}

void renderAudio(::ui::UiFrameContext &frame, const OptionsView &view) {
    ::ui::ProgressBar(frame, CLAY_ID("MasterVolume"), "Master volume", view.masterVolume, ::ui::color::Accent);
    ::ui::ProgressBar(frame, CLAY_ID("MusicVolume"), "Music volume", view.musicVolume, ::ui::color::Info);
    ::ui::Text(frame, "Volume bars are state-backed and ready to become slider primitives.", {.color = ::ui::color::TextMuted, .size = ::ui::type::Label, .font = ::ui::font::Body});
}

void renderVideo(::ui::UiFrameContext &frame, const OptionsView &view, ClientCommandQueue &queue) {
    ::ui::Button(frame, CLAY_ID("FullscreenToggle"), view.fullscreen ? "Fullscreen: On" : "Fullscreen: Off", ::ui::ButtonVariant::Secondary, [&] {
        queue.push({.type = ClientCommandType::ToggleFullscreen});
    });
    ::ui::Button(frame, CLAY_ID("VsyncToggle"), view.vsync ? "Vsync: On" : "Vsync: Off", ::ui::ButtonVariant::Secondary, [&] {
        queue.push({.type = ClientCommandType::ToggleVsync});
    });
    ::ui::Text(frame, "Layout responds to window resizing through Clay_SetLayoutDimensions each frame.", {.color = ::ui::color::TextMuted, .size = ::ui::type::Label, .font = ::ui::font::Body});
}

}

void renderOptions(::ui::UiFrameContext &frame, const OptionsView &view, ClientCommandQueue &queue) {
    screens::RootShell(frame, CLAY_ID("OptionsRoot"), [&] {
        ::ui::Box(frame, {
            .id = CLAY_ID("OptionsHeader"),
            .layout = {
                .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIT(0)},
                .childGap = 16,
                .childAlignment = {.y = CLAY_ALIGN_Y_CENTER}
            }
        }, [&] {
            ::ui::Button(frame, CLAY_ID("OptionsBack"), "Back", ::ui::ButtonVariant::Ghost, [&] {
                queue.push({.type = ClientCommandType::BackToMenu});
            });
            ::ui::Heading(frame, "Options");
        });

        ::ui::Box(frame, {
            .id = CLAY_ID("OptionsBody"),
            .layout = {
                .sizing = ::ui::grow(),
                .childGap = 18
            }
        }, [&] {
            ::ui::Panel(frame, CLAY_ID("OptionsTabs"), {.width = CLAY_SIZING_FIXED(220), .height = CLAY_SIZING_GROW(0)}, [&] {
                screens::options::renderSettingsTabs(frame, view.activeTab, queue);
            }, 12, 10);

            ::ui::Panel(frame, CLAY_ID("OptionsPanel"), ::ui::grow(), [&] {
                if (view.activeTab == OptionsTab::Controls) {
                    renderControls(frame);
                } else if (view.activeTab == OptionsTab::Audio) {
                    renderAudio(frame, view);
                } else {
                    renderVideo(frame, view, queue);
                }
            }, 18, 14);
        });
    });
}

}
