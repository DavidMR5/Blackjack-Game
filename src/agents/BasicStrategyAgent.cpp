#include "agents/BasicStrategyAgent.hpp"

namespace blackjack {

namespace {

Action softTotal(int player, int dealer) {
    if (player >= 19) {
        return Action::Stand;
    }

    if (player == 18) {
        return dealer <= 8 ? Action::Stand : Action::Hit;
    }

    return Action::Hit;
}

Action hardTotal(int player, int dealer) {
    if (player >= 17) {
        return Action::Stand;
    }

    if (player >= 13) {
        return dealer <= 6 ? Action::Stand : Action::Hit;
    }

    if (player == 12) {
        return (dealer >= 4 && dealer <= 6) ? Action::Stand : Action::Hit;
    }

    return Action::Hit;
}

}

Action BasicStrategyAgent::chooseAction(const State& state) {
    return state.usableAce
        ? softTotal(state.playerSum, state.dealerUpCard)
        : hardTotal(state.playerSum, state.dealerUpCard);
}

}
