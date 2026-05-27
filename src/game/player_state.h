#pragma once

namespace shooter {

struct PlayerState {
    int health  = 86;
    int armor   = 42;
    int credits = 450;

    bool can_afford(int cost) const { return credits >= cost; }
    void spend(int cost)            { credits -= cost; }
};

} // namespace shooter
