#pragma once

#include "client/ClientState.h"
#include "client/commands/ClientCommandQueue.h"
#include "platform/sdl/SdlPlatform.h"

namespace client {

class ClientCommandRouter {
public:
    ClientCommandRouter(ClientState &state, platform::sdl::SdlPlatform &platform, bool &running);

    void handle(const ClientCommand &command);
    void drain(ClientCommandQueue &queue);

private:
    ClientState &state_;
    platform::sdl::SdlPlatform &platform_;
    bool &running_;

    std::string &focusedText();
    void connectToLobby();
    void sendChat();
};

}
