#pragma once

#include "Agent.hpp"

namespace blackjack {

class BasicStrategyAgent : public Agent {
public:
    Action chooseAction(const State& state) override;
};

}
