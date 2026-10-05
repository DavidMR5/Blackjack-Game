#pragma once

#include "Agent.hpp"
#include "BasicStrategyAgent.hpp"

#include <cstdint>
#include <random>

namespace blackjack {

class MonteCarloAgent : public Agent {
public:
    explicit MonteCarloAgent(
        int simulationsPerAction = 1000,
        Rules rules = {},
        std::uint64_t seed = std::random_device{}()
    );

    Action chooseAction(const State& state) override;

    double estimateValue(const State& state, Action action);

private:
    double simulate(
        const State& state,
        Action action,
        std::uint64_t playerSeed,
        std::uint64_t dealerSeed
    );

    int simulationsPerAction_;
    Rules rules_;
    std::mt19937_64 rng_;
    BasicStrategyAgent rolloutPolicy_;
};

}
