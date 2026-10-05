#pragma once

#include "blackjack/Types.hpp"

namespace blackjack {

class Agent {
public:
    virtual ~Agent() = default;

    virtual Action chooseAction(const State& state) = 0;

    virtual void learn(
        const State& /*state*/,
        Action /*action*/,
        double /*reward*/,
        const State& /*nextState*/,
        bool /*terminal*/
    ) {}

    virtual void endEpisode() {}
};

}
