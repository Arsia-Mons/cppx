#pragma once

#include "client/ClientState.h"
#include "client/ui/navigation/ScreenId.h"
#include "client/ui/screens/options/OptionsState.h"

#include <string>

namespace client {

enum class ClientCommandType {
    Navigate,
    Quit,
    SetOptionsTab,
    SetFocusedField,
    AppendText,
    BackspaceFocusedField,
    Submit,
    AdvanceLoginFocus,
    Connect,
    SelectServer,
    SelectCharacter,
    SendChat,
    CreateGame,
    EnterGame,
    BackToMenu,
    ToggleFullscreen,
    ToggleVsync,
    AdjustMasterVolume,
    AdjustMusicVolume,
};

struct ClientCommand {
    ClientCommandType type{};
    ui::navigation::ScreenId screen{};
    ui::screens::options::OptionsTab tab{};
    ClientTextField field{};
    int index{};
    float value{};
    std::string text;
};

}
