#include "client/commands/ClientCommandRouter.h"

#include <algorithm>

namespace client {
namespace {

using ScreenId = ui::navigation::ScreenId;

int clampIndex(int value, int size) {
    if (size <= 0) {
        return 0;
    }
    return std::clamp(value, 0, size - 1);
}

}

ClientCommandRouter::ClientCommandRouter(ClientState &state, platform::sdl::SdlPlatform &platform, bool &running)
    : state_(state), platform_(platform), running_(running) {}

void ClientCommandRouter::handle(const ClientCommand &command) {
    switch (command.type) {
        case ClientCommandType::Navigate:
            state_.screens.navigate(command.screen);
            state_.focusedField = ClientTextField::None;
            break;
        case ClientCommandType::Quit:
            running_ = false;
            break;
        case ClientCommandType::SetOptionsTab:
            state_.optionsTab = command.tab;
            break;
        case ClientCommandType::SetFocusedField:
            state_.focusedField = command.field;
            break;
        case ClientCommandType::AppendText:
            if (state_.focusedField != ClientTextField::None && focusedText().size() < 80) {
                focusedText() += command.text;
            }
            break;
        case ClientCommandType::BackspaceFocusedField:
            if (state_.focusedField != ClientTextField::None && !focusedText().empty()) {
                focusedText().pop_back();
            }
            break;
        case ClientCommandType::Submit:
            if (state_.screens.active() == ScreenId::Connect) {
                connectToLobby();
            } else if (state_.focusedField == ClientTextField::Chat) {
                sendChat();
            }
            break;
        case ClientCommandType::AdvanceLoginFocus:
            state_.focusedField = state_.focusedField == ClientTextField::Username ? ClientTextField::Password : ClientTextField::Username;
            break;
        case ClientCommandType::Connect:
            connectToLobby();
            break;
        case ClientCommandType::SelectServer:
            state_.selectedServer = clampIndex(command.index, static_cast<int>(state_.servers.size()));
            break;
        case ClientCommandType::SelectCharacter:
            state_.selectedCharacter = clampIndex(command.index, static_cast<int>(state_.characters.size()));
            break;
        case ClientCommandType::SendChat:
            sendChat();
            break;
        case ClientCommandType::CreateGame:
            state_.chat.push_back({"system", "Created game: " + state_.gameName});
            break;
        case ClientCommandType::EnterGame:
            state_.screens.navigate(ScreenId::Game);
            state_.focusedField = ClientTextField::None;
            break;
        case ClientCommandType::BackToMenu:
            state_.screens.backToMenu();
            state_.focusedField = ClientTextField::None;
            break;
        case ClientCommandType::ToggleFullscreen:
            state_.fullscreen = !state_.fullscreen;
            platform_.setFullscreen(state_.fullscreen);
            break;
        case ClientCommandType::ToggleVsync:
            state_.vsync = !state_.vsync;
            platform_.setVsync(state_.vsync);
            break;
        case ClientCommandType::AdjustMasterVolume:
            state_.masterVolume = std::clamp(command.value, 0.0f, 1.0f);
            break;
        case ClientCommandType::AdjustMusicVolume:
            state_.musicVolume = std::clamp(command.value, 0.0f, 1.0f);
            break;
    }
}

void ClientCommandRouter::drain(ClientCommandQueue &queue) {
    for (const ClientCommand &command : queue.drain()) {
        handle(command);
    }
}

std::string &ClientCommandRouter::focusedText() {
    switch (state_.focusedField) {
        case ClientTextField::Username:
            return state_.username;
        case ClientTextField::Password:
            return state_.password;
        case ClientTextField::Chat:
            return state_.chatDraft;
        case ClientTextField::GameName:
            return state_.gameName;
        case ClientTextField::None:
            break;
    }
    return state_.chatDraft;
}

void ClientCommandRouter::connectToLobby() {
    state_.screens.navigate(ScreenId::Lobby);
    state_.focusedField = ClientTextField::Chat;
    state_.chat.push_back({"system", state_.username + " connected."});
}

void ClientCommandRouter::sendChat() {
    if (!state_.chatDraft.empty()) {
        state_.chat.push_back({state_.username.empty() ? "player" : state_.username, state_.chatDraft});
        state_.chatDraft.clear();
    }
}

}
