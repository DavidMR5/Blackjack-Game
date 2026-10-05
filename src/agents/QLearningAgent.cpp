#include "agents/QLearningAgent.hpp"

#include <algorithm>
#include <cassert>
#include <iomanip>
#include <ostream>
#include <string>

namespace blackjack {

QLearningAgent::QLearningAgent(
    double minLearningRate,
    double discountFactor,
    double explorationRate,
    std::uint32_t seed
)
    : minLearningRate_(minLearningRate),
      discountFactor_(discountFactor),
      explorationRate_(explorationRate),
      rng_(seed) {}

std::size_t QLearningAgent::indexOf(const State& state) {
    assert(state.playerSum >= 0 && state.playerSum < kSums);
    assert(state.dealerUpCard >= 0 && state.dealerUpCard < kDealer);

    const std::size_t soft = state.usableAce ? 1 : 0;

    return (soft * kDealer + static_cast<std::size_t>(state.dealerUpCard)) * kSums
         + static_cast<std::size_t>(state.playerSum);
}

std::size_t QLearningAgent::actionIndex(Action action) {
    return action == Action::Hit ? 0 : 1;
}

Action QLearningAgent::getGreedyAction(const State& state) const {
    const QValues& q = qTable_[indexOf(state)];
    return q[0] > q[1] ? Action::Hit : Action::Stand;
}

double QLearningAgent::getQValue(const State& state, Action action) const {
    return qTable_[indexOf(state)][actionIndex(action)];
}

void QLearningAgent::setExplorationRate(double epsilon) {
    explorationRate_ = epsilon;
}

Action QLearningAgent::chooseAction(const State& state) {
    std::uniform_real_distribution<double> probability(0.0, 1.0);

    if (probability(rng_) < explorationRate_) {
        return std::bernoulli_distribution(0.5)(rng_) ? Action::Hit : Action::Stand;
    }

    return getGreedyAction(state);
}

void QLearningAgent::learn(
    const State& state,
    Action action,
    double reward,
    const State& nextState,
    bool terminal
) {
    const std::size_t s = indexOf(state);
    const std::size_t a = actionIndex(action);

    double target = reward;

    if (!terminal) {
        const QValues& next = qTable_[indexOf(nextState)];
        target += discountFactor_ * std::max(next[0], next[1]);
    }

    const std::uint32_t n = ++visits_[s][a];
    // Running average (1/N) with a floor.
    const double alpha = std::max(minLearningRate_, 1.0 / n);

    qTable_[s][a] += alpha * (target - qTable_[s][a]);
}

void QLearningAgent::printPolicy(std::ostream& out) const {
    auto printChart = [&](bool soft, int fromSum, int toSum) {
        out << (soft ? "\nSoft totals" : "\nHard totals")
            << "      dealer up card\n      ";

        for (int dealer = 2; dealer <= 11; ++dealer) {
            out << std::setw(3) << (dealer == 11 ? "A" : std::to_string(dealer));
        }

        out << '\n';

        for (int sum = toSum; sum >= fromSum; --sum) {
            out << std::setw(4) << sum << "  ";

            for (int dealer = 2; dealer <= 11; ++dealer) {
                const State state{sum, dealer, soft};
                const bool visited = visits_[indexOf(state)][0] + visits_[indexOf(state)][1] > 0;
                const char symbol = !visited
                    ? '.'
                    : (getGreedyAction(state) == Action::Hit ? 'H' : 'S');

                out << "  " << symbol;
            }

            out << '\n';
        }
    };

    printChart(false, 4, 20);
    printChart(true, 13, 20);
}

}
