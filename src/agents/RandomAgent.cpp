#include "agents/RandomAgent.hpp"

namespace blackjack {

RandomAgent::RandomAgent(std::uint32_t seed)
    : rng_(seed) {}

Action RandomAgent::chooseAction(const State&) {
    return coin_(rng_) ? Action::Hit : Action::Stand;
}

}
