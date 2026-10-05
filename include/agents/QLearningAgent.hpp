#pragma once

#include "Agent.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <iosfwd>
#include <random>

namespace blackjack {

class QLearningAgent : public Agent {
public:
    explicit QLearningAgent(
        double minLearningRate = 0.001,
        double discountFactor = 1.0,
        double explorationRate = 0.3,
        std::uint32_t seed = std::random_device{}()
    );

    Action chooseAction(const State& state) override;

    void learn(
        const State& state,
        Action action,
        double reward,
        const State& nextState,
        bool terminal
    ) override;

    Action getGreedyAction(const State& state) const;
    double getQValue(const State& state, Action action) const;

    void setExplorationRate(double epsilon);

    void printPolicy(std::ostream& out) const;

private:
    static constexpr int kSums = 32;
    static constexpr int kDealer = 12;
    static constexpr int kSoft = 2;
    static constexpr std::size_t kStates =
        static_cast<std::size_t>(kSums) * kDealer * kSoft;

    using QValues = std::array<double, 2>;
    using Visits = std::array<std::uint32_t, 2>;

    static std::size_t indexOf(const State& state);
    static std::size_t actionIndex(Action action);

    double minLearningRate_;
    double discountFactor_;
    double explorationRate_;

    // Flat table indexed by state: no hashing, fits in L1.
    std::array<QValues, kStates> qTable_{};
    std::array<Visits, kStates> visits_{};

    std::mt19937 rng_;
};

}
