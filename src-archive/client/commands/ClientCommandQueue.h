#pragma once

#include "client/commands/ClientCommand.h"

#include <vector>

namespace client {

class ClientCommandQueue {
public:
    void push(ClientCommand command) {
        pending_.push_back(std::move(command));
    }

    std::vector<ClientCommand> drain() {
        std::vector<ClientCommand> copy;
        copy.swap(pending_);
        return copy;
    }

private:
    std::vector<ClientCommand> pending_;
};

}
