#pragma once

#include "Agent.hpp"

#include <iosfwd>

namespace blackjack {

class HumanAgent : public Agent {
public:
    HumanAgent(std::istream& in, std::ostream& out, Agent* advisor = nullptr);

    Action chooseAction(const State& state) override;

    bool wantsToQuit() const;

private:
    std::istream& in_;
    std::ostream& out_;
    Agent* advisor_;
    bool quit_ = false;
};

}
