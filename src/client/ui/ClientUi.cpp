#include "client/ui/ClientUi.h"

#include "client/ui/ClientUiView.h"
#include "client/ui/navigation/ScreenId.h"
#include "client/ui/screens/connect/ConnectScreen.h"
#include "client/ui/screens/game/GameScreen.h"
#include "client/ui/screens/lobby/LobbyScreen.h"
#include "client/ui/screens/main_menu/MainMenuScreen.h"
#include "client/ui/screens/options/OptionsScreen.h"

namespace client::ui {

void render(::ui::UiFrameContext &frame, const ClientState &state, ClientCommandQueue &queue) {
    switch (state.screens.active()) {
        case navigation::ScreenId::MainMenu:
            renderMainMenu(frame, MainMenuView{}, queue);
            break;
        case navigation::ScreenId::Options:
            renderOptions(frame, {
                .activeTab = state.optionsTab,
                .fullscreen = state.fullscreen,
                .vsync = state.vsync,
                .masterVolume = state.masterVolume,
                .musicVolume = state.musicVolume
            }, queue);
            break;
        case navigation::ScreenId::Connect:
            renderConnect(frame, {
                .username = state.username,
                .password = state.password,
                .focusedField = state.focusedField
            }, queue);
            break;
        case navigation::ScreenId::Lobby:
            renderLobby(frame, {
                .servers = {state.servers.data(), state.servers.size()},
                .characters = {state.characters.data(), state.characters.size()},
                .chat = {state.chat.data(), state.chat.size()},
                .username = state.username,
                .chatDraft = state.chatDraft,
                .gameName = state.gameName,
                .selectedServer = state.selectedServer,
                .selectedCharacter = state.selectedCharacter,
                .focusedField = state.focusedField
            }, queue);
            break;
        case navigation::ScreenId::Game:
            renderGame(frame, {
                .server = state.servers[state.selectedServer],
                .character = state.characters[state.selectedCharacter]
            }, queue);
            break;
    }
}

}
