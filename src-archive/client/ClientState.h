#pragma once

#include "client/ui/navigation/ScreenStack.h"
#include "client/ui/screens/options/OptionsState.h"

#include <array>
#include <string>
#include <vector>

namespace client {

enum class ClientTextField {
    None,
    Username,
    Password,
    Chat,
    GameName,
};

struct ChatMessage {
    std::string author;
    std::string body;
};

struct ClientState {
    ui::navigation::ScreenStack screens;
    ui::screens::options::OptionsTab optionsTab = ui::screens::options::OptionsTab::Controls;
    ClientTextField focusedField = ClientTextField::None;

    std::string username = "pilot";
    std::string password;
    std::string chatDraft;
    std::string gameName = "Training Room";

    int selectedServer = 0;
    int selectedCharacter = 0;
    bool fullscreen = false;
    bool vsync = true;
    float masterVolume = 0.74f;
    float musicVolume = 0.52f;

    std::array<std::string, 4> servers{
        "NA West / Low Ping",
        "NA East / Ranked",
        "EU Central / Social",
        "Localhost Dev"
    };

    std::array<std::string, 4> characters{
        "Vanguard",
        "Signal",
        "Mender",
        "Shade"
    };

    std::vector<ChatMessage> chat{
        {"system", "Welcome to the reference lobby."},
        {"mira", "Server select, character select, and creation are all live."},
        {"dev", "Scroll this pane to verify Clay clipping and wheel input."}
    };
};

}
