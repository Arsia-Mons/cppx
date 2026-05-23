#pragma once

#include "client/ClientState.h"

#include <span>
#include <string>

namespace client::ui {

struct MainMenuView {};

struct ConnectView {
    const std::string &username;
    const std::string &password;
    ClientTextField focusedField;
};

struct OptionsView {
    screens::options::OptionsTab activeTab;
    bool fullscreen = false;
    bool vsync = true;
    float masterVolume = 0.0f;
    float musicVolume = 0.0f;
};

struct LobbyView {
    std::span<const std::string> servers;
    std::span<const std::string> characters;
    std::span<const ChatMessage> chat;
    const std::string &username;
    const std::string &chatDraft;
    const std::string &gameName;
    int selectedServer = 0;
    int selectedCharacter = 0;
    ClientTextField focusedField = ClientTextField::None;
};

struct GameView {
    const std::string &server;
    const std::string &character;
};

}
