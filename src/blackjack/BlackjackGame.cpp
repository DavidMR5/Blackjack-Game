#include "blackjack/BlackjackGame.hpp"

#include <stdexcept>
#include <utility>

namespace blackjack {

BlackjackGame::BlackjackGame(Rules rules, std::uint32_t seed)
    : rules_(rules), deck_(rules.numDecks, seed) {
    deck_.shuffle();
}

BlackjackGame::BlackjackGame(Rules rules, Deck deck)
    : rules_(rules), deck_(std::move(deck)) {}

const Rules& BlackjackGame::getRules() const {
    return rules_;
}

BlackjackGame::Phase BlackjackGame::getPhase() const {
    return phase_;
}

bool BlackjackGame::isAwaitingPlayer() const {
    return phase_ == Phase::AwaitingPlayer;
}

bool BlackjackGame::isRoundOver() const {
    return phase_ == Phase::RoundOver;
}

State BlackjackGame::getState() const {
    return state_;
}

const RoundResult& BlackjackGame::getResult() const {
    return result_;
}

const Hand& BlackjackGame::getPlayerHand() const {
    return player_;
}

const Hand& BlackjackGame::getDealerHand() const {
    return dealer_;
}

State BlackjackGame::createState(const Hand& player, const Card& dealerUpCard) {
    return State{
        player.getValue(),
        dealerUpCard.getValue(),
        player.hasUsableAce()
    };
}

void BlackjackGame::reshuffle() {
    deck_.reset();
    deck_.shuffle();

    if (observer_) {
        observer_->onShuffle();
    }
}

Card BlackjackGame::drawCard() {
    if (deck_.empty()) {
        reshuffle();
    }

    return deck_.draw();
}

bool BlackjackGame::dealerMustHit() const {
    const int value = dealer_.getValue();

    if (value < 17) {
        return true;
    }

    return value == 17 && rules_.dealerHitsSoft17 && dealer_.hasUsableAce();
}

void BlackjackGame::dealerPlays() {
    while (dealerMustHit()) {
        const Card card = drawCard();
        dealer_.addCard(card);

        if (observer_) {
            observer_->onDealerCard(card, dealer_);
        }
    }
}

void BlackjackGame::finish(GameResult result, double reward) {
    phase_ = Phase::RoundOver;

    result_.result = result;
    result_.reward = reward;
    result_.player = player_;
    result_.dealer = dealer_;

    if (observer_) {
        observer_->onDealerReveal(dealer_);
        observer_->onRoundEnd(result_);
    }
}

void BlackjackGame::beginRound(RoundObserver* observer) {
    observer_ = observer;

    if (deck_.needsReshuffle(rules_.penetration)) {
        reshuffle();
    }

    player_.clear();
    dealer_.clear();

    const bool peek = rules_.dealerPeeks;

    player_.addCard(drawCard());
    dealer_.addCard(drawCard());
    player_.addCard(drawCard());

    if (peek) {
        dealer_.addCard(drawCard());
    }

    const Card upCard = dealer_.getCards().front();

    if (observer_) {
        observer_->onInitialDeal(player_, upCard, peek);
    }

    if (peek) {
        if (player_.isBlackjack()) {
            if (dealer_.isBlackjack()) {
                finish(GameResult::Push, 0.0);
            } else {
                finish(GameResult::PlayerBlackjack, rules_.blackjackPayout);
            }
            return;
        }

        if (dealer_.isBlackjack()) {
            finish(GameResult::DealerBlackjack, -1.0);
            return;
        }
    // European rules: deal the 2nd dealer card only to check for a push.
    } else if (player_.isBlackjack()) {
        const Card card = drawCard();
        dealer_.addCard(card);

        if (observer_) {
            observer_->onDealerCard(card, dealer_);
        }

        if (dealer_.isBlackjack()) {
            finish(GameResult::Push, 0.0);
        } else {
            finish(GameResult::PlayerBlackjack, rules_.blackjackPayout);
        }
        return;
    }

    state_ = createState(player_, upCard);
    phase_ = Phase::AwaitingPlayer;
}

void BlackjackGame::act(Action action, Agent* learner) {
    if (phase_ != Phase::AwaitingPlayer) {
        throw std::logic_error("act() called while the game is not waiting for the player.");
    }

    if (action == Action::Stand) {
        settleAfterStand(learner);
        return;
    }

    const Card card = drawCard();
    player_.addCard(card);

    if (observer_) {
        observer_->onPlayerCard(card, player_);
    }

    const State nextState = createState(player_, dealer_.getCards().front());

    if (player_.isBust()) {
        if (learner) {
            learner->learn(state_, action, -1.0, nextState, true);
        }
        finish(GameResult::DealerWin, -1.0);
        return;
    }

    if (learner) {
        learner->learn(state_, action, 0.0, nextState, false);
    }
    state_ = nextState;

    if (player_.getValue() == 21) {
        settleAfterStand(learner);
    }
}

void BlackjackGame::settleAfterStand(Agent* learner) {
    // European rules: a dealer blackjack beats any 21 with 3+ cards.
    if (!rules_.dealerPeeks) {
        const Card card = drawCard();
        dealer_.addCard(card);

        if (observer_) {
            observer_->onDealerCard(card, dealer_);
        }

        if (dealer_.isBlackjack()) {
            if (learner) {
                learner->learn(state_, Action::Stand, -1.0, state_, true);
            }
            finish(GameResult::DealerBlackjack, -1.0);
            return;
        }
    }

    dealerPlays();

    GameResult result;
    double reward;

    if (dealer_.isBust() || player_.getValue() > dealer_.getValue()) {
        result = GameResult::PlayerWin;
        reward = 1.0;
    } else if (player_.getValue() < dealer_.getValue()) {
        result = GameResult::DealerWin;
        reward = -1.0;
    } else {
        result = GameResult::Push;
        reward = 0.0;
    }

    if (learner) {
        learner->learn(state_, Action::Stand, reward, state_, true);
    }

    finish(result, reward);
}

const RoundResult& BlackjackGame::playRound(Agent& agent, RoundObserver* observer) {
    beginRound(observer);

    while (isAwaitingPlayer()) {
        act(agent.chooseAction(state_), &agent);
    }

    agent.endEpisode();

    return result_;
}

}
