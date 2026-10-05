#pragma once

#include "Deck.hpp"
#include "Hand.hpp"
#include "RoundObserver.hpp"
#include "Types.hpp"
#include "agents/Agent.hpp"

#include <cstdint>
#include <random>

namespace blackjack {

struct RoundResult {
    GameResult result;
    double reward;
    Hand player;
    Hand dealer;
};

// State machine: beginRound() -> act()... -> getResult(). playRound() loops over it.
class BlackjackGame {
public:
    enum class Phase : std::uint8_t {
        Idle,
        AwaitingPlayer,
        RoundOver
    };

    explicit BlackjackGame(
        Rules rules = {},
        std::uint32_t seed = std::random_device{}()
    );

    BlackjackGame(Rules rules, Deck deck);

    const RoundResult& playRound(Agent& agent, RoundObserver* observer = nullptr);

    void beginRound(RoundObserver* observer = nullptr);

    void act(Action action, Agent* learner = nullptr);

    Phase getPhase() const;
    bool isAwaitingPlayer() const;
    bool isRoundOver() const;

    State getState() const;
    const RoundResult& getResult() const;
    const Hand& getPlayerHand() const;
    const Hand& getDealerHand() const;

    const Rules& getRules() const;

    static State createState(const Hand& player, const Card& dealerUpCard);

private:
    Card drawCard();
    void reshuffle();
    void dealerPlays();
    bool dealerMustHit() const;
    void finish(GameResult result, double reward);
    void settleAfterStand(Agent* learner);

    Rules rules_;
    Deck deck_;

    Phase phase_ = Phase::Idle;
    Hand player_;
    Hand dealer_;
    State state_{};
    RoundResult result_{GameResult::Push, 0.0, {}, {}};
    RoundObserver* observer_ = nullptr;
};

}
