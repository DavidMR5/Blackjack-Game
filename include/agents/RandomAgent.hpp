#pragma once

#include "Agent.hpp"

#include <cstdint>
#include <random>

namespace blackjack {

class RandomAgent : public Agent {
public:
    explicit RandomAgent(std::uint32_t seed = std::random_device{}());

    Action chooseAction(const State& state) override;

private:
    std::mt19937 rng_;
    std::bernoulli_distribution coin_{0.5};
};

}
