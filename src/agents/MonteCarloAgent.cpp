#include "agents/MonteCarloAgent.hpp"

namespace blackjack {

namespace {

class SplitMix64 {
public:
    explicit SplitMix64(std::uint64_t seed) : state_(seed) {}

    std::uint64_t next() {
        std::uint64_t z = (state_ += 0x9E3779B97F4A7C15ULL);
        z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
        z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
        return z ^ (z >> 31);
    }

    int drawCardValue() {
        const int rank = static_cast<int>(next() % 13) + 1;

        if (rank == 1) {
            return 11;
        }

        return rank >= 10 ? 10 : rank;
    }

private:
    std::uint64_t state_;
};

struct SimHand {
    int total = 0;
    int softAces = 0;

    void add(int value) {
        total += value;

        if (value == 11) {
            ++softAces;
        }

        while (total > 21 && softAces > 0) {
            total -= 10;
            --softAces;
        }
    }

    bool isSoft() const { return softAces > 0; }
};

}

MonteCarloAgent::MonteCarloAgent(
    int simulationsPerAction,
    Rules rules,
    std::uint64_t seed
)
    : simulationsPerAction_(simulationsPerAction),
      rules_(rules),
      rng_(seed) {}

double MonteCarloAgent::estimateValue(const State& state, Action action) {
    double total = 0.0;

    for (int i = 0; i < simulationsPerAction_; ++i) {
        total += simulate(state, action, rng_(), rng_());
    }

    return total / simulationsPerAction_;
}

Action MonteCarloAgent::chooseAction(const State& state) {
    double hitTotal = 0.0;
    double standTotal = 0.0;

    for (int i = 0; i < simulationsPerAction_; ++i) {
        // Same seeds for Hit and Stand (common random numbers).
        const std::uint64_t playerSeed = rng_();
        const std::uint64_t dealerSeed = rng_();

        hitTotal += simulate(state, Action::Hit, playerSeed, dealerSeed);
        standTotal += simulate(state, Action::Stand, playerSeed, dealerSeed);
    }

    return hitTotal > standTotal ? Action::Hit : Action::Stand;
}

double MonteCarloAgent::simulate(
    const State& state,
    Action action,
    std::uint64_t playerSeed,
    std::uint64_t dealerSeed
) {
    SplitMix64 playerCards(playerSeed);
    SplitMix64 dealerCards(dealerSeed);

    SimHand player{state.playerSum, state.usableAce ? 1 : 0};

    int holeCard = dealerCards.drawCardValue();

    // The dealer already peeked: the hole card cannot make a blackjack.
    if (rules_.dealerPeeks) {
        while (state.dealerUpCard + holeCard == 21) {
            holeCard = dealerCards.drawCardValue();
        }
    }

    if (action == Action::Hit) {
        player.add(playerCards.drawCardValue());

        while (player.total < 21) {
            const State next{player.total, state.dealerUpCard, player.isSoft()};

            if (rolloutPolicy_.chooseAction(next) == Action::Stand) {
                break;
            }

            player.add(playerCards.drawCardValue());
        }

        if (player.total > 21) {
            return -1.0;
        }
    }

    if (state.dealerUpCard + holeCard == 21) {
        return -1.0;
    }

    SimHand dealer;
    dealer.add(state.dealerUpCard);
    dealer.add(holeCard);

    while (dealer.total < 17 ||
           (dealer.total == 17 && rules_.dealerHitsSoft17 && dealer.isSoft())) {
        dealer.add(dealerCards.drawCardValue());
    }

    if (dealer.total > 21 || player.total > dealer.total) {
        return 1.0;
    }

    if (player.total < dealer.total) {
        return -1.0;
    }

    return 0.0;
}

}
