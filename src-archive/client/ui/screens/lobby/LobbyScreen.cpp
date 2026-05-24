#include "client/ui/screens/lobby/LobbyScreen.h"

#include "client/commands/ClientCommand.h"
#include "client/ui/screens/ScreenLayout.h"
#include "client/ui/screens/lobby/components/CharacterList.h"
#include "client/ui/screens/lobby/components/ChatPanel.h"
#include "client/ui/screens/lobby/components/CreateGamePanel.h"
#include "client/ui/screens/lobby/components/ServerList.h"
#include "ui/layout/Box.h"
#include "ui/primitives/Button.h"
#include "ui/primitives/Panel.h"
#include "ui/primitives/Text.h"

namespace client::ui {

void renderLobby(::ui::UiFrameContext &frame, const LobbyView &view, ClientCommandQueue &queue) {
    screens::RootShell(frame, CLAY_ID("LobbyRoot"), [&] {
        ::ui::Box(frame, {
            .id = CLAY_ID("LobbyHeader"),
            .layout = {
                .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIT(0)},
                .childGap = 16,
                .childAlignment = {.y = CLAY_ALIGN_Y_CENTER}
            }
        }, [&] {
            ::ui::Heading(frame, "Lobby");
            ::ui::Button(frame, CLAY_ID("Play"), "Enter Game", ::ui::ButtonVariant::Primary, [&] {
                queue.push({.type = ClientCommandType::EnterGame});
            });
            ::ui::Button(frame, CLAY_ID("LobbyBack"), "Menu", ::ui::ButtonVariant::Ghost, [&] {
                queue.push({.type = ClientCommandType::BackToMenu});
            });
        });

        ::ui::Box(frame, {
            .id = CLAY_ID("LobbyGrid"),
            .layout = {
                .sizing = ::ui::grow(),
                .childGap = 18
            }
        }, [&] {
            ::ui::Box(frame, {
                .id = CLAY_ID("LobbyLeft"),
                .layout = {
                    .sizing = {.width = CLAY_SIZING_FIXED(280), .height = CLAY_SIZING_GROW(0)},
                    .childGap = 18,
                    .layoutDirection = CLAY_TOP_TO_BOTTOM
                }
            }, [&] {
                ::ui::Panel(frame, CLAY_ID("ServersPanel"), {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_PERCENT(0.5f)}, [&] {
                    screens::lobby::renderServerList(frame, view, queue);
                }, 14, 10);

                ::ui::Panel(frame, CLAY_ID("CharacterPanel"), {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_PERCENT(0.5f)}, [&] {
                    screens::lobby::renderCharacterList(frame, view, queue);
                }, 14, 10);
            });

            ::ui::Panel(frame, CLAY_ID("ChatPanel"), ::ui::grow(), [&] {
                screens::lobby::renderChatPanel(frame, view, queue);
            }, 14, 10);

            ::ui::Panel(frame, CLAY_ID("GameCreationPanel"), {.width = CLAY_SIZING_FIXED(300), .height = CLAY_SIZING_GROW(0)}, [&] {
                screens::lobby::renderCreateGamePanel(frame, view, queue);
            }, 14, 12);
        });
    });
}

}
