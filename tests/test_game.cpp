#include "TestFramework.hpp"

#include "agents/BasicStrategyAgent.hpp"
#include "agents/RandomAgent.hpp"
#include "blackjack/BlackjackGame.hpp"

#include <cmath>
#include <cstddef>
#include <utility>
#include <vector>

using namespace blackjack;

namespace {

class ScriptedAgent : public Agent {
public:
    struct LearnCall {
        State state;
        Action action;
        double reward;
        bool terminal;
    };

    explicit ScriptedAgent(std::vector<Action> actions = {})
        : actions_(std::move(actions)) {}

    Action chooseAction(const State& state) override {
        seenStates.push_back(state);
        return decisions < actions_.size() ? actions_[decisions++] : (++decisions, Action::Hit);
    }

    void learn(const State& state, Action action, double reward,
               const State&, bool terminal) override {
        learnCalls.push_back({state, action, reward, terminal});
    }

    void endEpisode() override { ++episodes; }

    std::size_t decisions = 0;
    int episodes = 0;
    std::vector<State> seenStates;
    std::vector<LearnCall> learnCalls;

private:
    std::vector<Action> actions_;
};

Card c(Rank rank) {
    return Card(Suit::Clubs, rank);
}

BlackjackGame stackedGame(std::vector<Card> cards, Rules rules = {}) {
    rules.penetration = 1.0;
    return BlackjackGame(rules, Deck::stacked(cards));
}

void testPlayerBlackjackPaysThreeToTwo() {
    auto game = stackedGame({c(Rank::Ace), c(Rank::Nine), c(Rank::King), c(Rank::Seven)});
    ScriptedAgent agent;

    const RoundResult round = game.playRound(agent);

    CHECK(round.result == GameResult::PlayerBlackjack);
    CHECK(round.reward == 1.5);
    CHECK(agent.decisions == 0);
    CHECK(agent.learnCalls.empty());
    CHECK(agent.episodes == 1);
}

void testBothBlackjackIsPush() {
    auto game = stackedGame({c(Rank::Ace), c(Rank::Ace), c(Rank::King), c(Rank::Queen)});
    ScriptedAgent agent;

    const RoundResult round = game.playRound(agent);

    CHECK(round.result == GameResult::Push);
    CHECK(round.reward == 0.0);
}

void testDealerBlackjackLosesImmediately() {
    auto game = stackedGame({c(Rank::Ten), c(Rank::Ace), c(Rank::Nine), c(Rank::King)});
    ScriptedAgent agent;

    const RoundResult round = game.playRound(agent);

    CHECK(round.result == GameResult::DealerBlackjack);
    CHECK(round.reward == -1.0);
    CHECK(agent.decisions == 0);
    CHECK(agent.episodes == 1);
}

void testPlayerBustEndsRound() {
    auto game = stackedGame({
        c(Rank::Ten), c(Rank::Six), c(Rank::Six), c(Rank::Ten),
        c(Rank::King)
    });
    ScriptedAgent agent({Action::Hit});

    const RoundResult round = game.playRound(agent);

    CHECK(round.reward == -1.0);
    CHECK(round.player.isBust());
    CHECK(round.dealer.size() == 2);
    CHECK(agent.learnCalls.size() == 1);
    CHECK(agent.learnCalls[0].reward == -1.0);
    CHECK(agent.learnCalls[0].terminal);
}

void testStandReceivesTerminalReward() {
    auto game = stackedGame({c(Rank::Ten), c(Rank::Ten), c(Rank::Nine), c(Rank::Seven)});
    ScriptedAgent agent({Action::Stand});

    const RoundResult round = game.playRound(agent);

    CHECK(round.result == GameResult::PlayerWin);
    CHECK(round.reward == 1.0);
    CHECK(agent.learnCalls.size() == 1);
    CHECK(agent.learnCalls[0].action == Action::Stand);
    CHECK(agent.learnCalls[0].reward == 1.0);
    CHECK(agent.learnCalls[0].terminal);
    CHECK((agent.learnCalls[0].state == State{19, 10, false}));
}

void testDealerBust() {
    auto game = stackedGame({
        c(Rank::Ten), c(Rank::Six), c(Rank::Two), c(Rank::Ten),
        c(Rank::Queen)
    });
    ScriptedAgent agent({Action::Stand});

    const RoundResult round = game.playRound(agent);

    CHECK(round.dealer.isBust());
    CHECK(round.reward == 1.0);
}

void testPush() {
    auto game = stackedGame({c(Rank::Ten), c(Rank::Ten), c(Rank::Eight), c(Rank::Eight)});
    ScriptedAgent agent({Action::Stand});

    const RoundResult round = game.playRound(agent);

    CHECK(round.result == GameResult::Push);
    CHECK(round.reward == 0.0);
}

void testDealerSoft17Rule() {
    const std::vector<Card> cards = {
        c(Rank::Ten), c(Rank::Ace), c(Rank::Nine), c(Rank::Six), c(Rank::Four)
    };

    {
        auto game = stackedGame(cards, Rules{.dealerHitsSoft17 = false});
        ScriptedAgent agent({Action::Stand});
        const RoundResult round = game.playRound(agent);

        CHECK(round.dealer.getValue() == 17);
        CHECK(round.reward == 1.0);
        CHECK(agent.seenStates.at(0).dealerUpCard == 11);
    }

    {
        auto game = stackedGame(cards, Rules{.dealerHitsSoft17 = true});
        ScriptedAgent agent({Action::Stand});
        const RoundResult round = game.playRound(agent);

        CHECK(round.dealer.getValue() == 21);
        CHECK(round.reward == -1.0);
    }
}

void testAutoStandOn21() {
    auto game = stackedGame({
        c(Rank::Five), c(Rank::Ten), c(Rank::Six), c(Rank::Seven),
        c(Rank::Ten)
    });
    ScriptedAgent agent;

    const RoundResult round = game.playRound(agent);

    CHECK(agent.decisions == 1);
    CHECK(round.player.getValue() == 21);
    CHECK(round.reward == 1.0);
}

constexpr Rules kEuropean{.dealerPeeks = false};

void testEuropeanPlayerActsBeforeDealerBlackjack() {
    auto game = stackedGame({
        c(Rank::Ten), c(Rank::Ace), c(Rank::Five),
        c(Rank::Two),
        c(Rank::King)
    }, kEuropean);
    ScriptedAgent agent({Action::Hit, Action::Stand});

    const RoundResult round = game.playRound(agent);

    CHECK(agent.decisions == 2);
    CHECK(round.result == GameResult::DealerBlackjack);
    CHECK(round.reward == -1.0);
    CHECK(agent.learnCalls.size() == 2);
    CHECK(agent.learnCalls.back().terminal);
}

void testEuropeanDealerBlackjackBeatsThreeCard21() {
    auto game = stackedGame({
        c(Rank::Five), c(Rank::Ten), c(Rank::Six),
        c(Rank::Ten),
        c(Rank::Ace)
    }, kEuropean);
    ScriptedAgent agent;

    const RoundResult round = game.playRound(agent);

    CHECK(round.player.getValue() == 21);
    CHECK(round.result == GameResult::DealerBlackjack);
    CHECK(round.reward == -1.0);
}

void testEuropeanBlackjackAgainstBlackjackIsPush() {
    auto game = stackedGame({c(Rank::Ace), c(Rank::King), c(Rank::Queen), c(Rank::Ace)}, kEuropean);
    ScriptedAgent agent;

    const RoundResult round = game.playRound(agent);

    CHECK(round.result == GameResult::Push);
    CHECK(agent.decisions == 0);
}

void testEuropeanPlayerBlackjackPays() {
    auto game = stackedGame({c(Rank::Ace), c(Rank::King), c(Rank::Queen), c(Rank::Five)}, kEuropean);
    ScriptedAgent agent;

    const RoundResult round = game.playRound(agent);

    CHECK(round.result == GameResult::PlayerBlackjack);
    CHECK(round.reward == 1.5);
}

void testEuropeanNormalRound() {
    auto game = stackedGame({
        c(Rank::Ten), c(Rank::Ten), c(Rank::Nine),
        c(Rank::Seven)
    }, kEuropean);
    ScriptedAgent agent({Action::Stand});

    const RoundResult round = game.playRound(agent);

    CHECK(round.reward == 1.0);
    CHECK(round.dealer.size() == 2);
}

void testEuropeanSameExpectedValue() {
    BasicStrategyAgent agent;
    BlackjackGame american(Rules{}, 11);
    BlackjackGame european(kEuropean, 11);

    double a = 0.0;
    double e = 0.0;
    constexpr int kRounds = 400'000;

    for (int i = 0; i < kRounds; ++i) {
        a += american.playRound(agent).reward;
        e += european.playRound(agent).reward;
    }

    CHECK(std::abs(a - e) / kRounds < 0.01);
}

void testStepApiMatchesPlayRound() {
    BasicStrategyAgent agent;
    BlackjackGame blocking(Rules{}, 21);
    BlackjackGame stepped(Rules{}, 21);

    bool identical = true;
    for (int i = 0; i < 5000; ++i) {
        const double expected = blocking.playRound(agent).reward;

        stepped.beginRound();
        while (stepped.isAwaitingPlayer()) {
            stepped.act(agent.chooseAction(stepped.getState()));
        }

        identical = identical && stepped.isRoundOver() &&
                    stepped.getResult().reward == expected;
    }

    CHECK(identical);
}

void testActOutsidePlayerTurnThrows() {
    BlackjackGame game(Rules{}, 1);

    CHECK(game.getPhase() == BlackjackGame::Phase::Idle);
    CHECK_THROWS(game.act(Action::Hit));
}

void testStepByStepHitThenStand() {
    auto game = stackedGame({
        c(Rank::Ten), c(Rank::Ten), c(Rank::Two), c(Rank::Seven),
        c(Rank::Five)
    });

    game.beginRound();
    CHECK(game.isAwaitingPlayer());
    CHECK((game.getState() == State{12, 10, false}));

    game.act(Action::Hit);
    CHECK(game.isAwaitingPlayer());
    CHECK(game.getPlayerHand().getValue() == 17);

    game.act(Action::Stand);
    CHECK(game.isRoundOver());
    CHECK(game.getResult().result == GameResult::Push);
    CHECK_THROWS(game.act(Action::Hit));
}

void testSameSeedIsReproducible() {
    BlackjackGame a(Rules{}, 99);
    BlackjackGame b(Rules{}, 99);
    BasicStrategyAgent agent;

    bool identical = true;
    for (int i = 0; i < 5000; ++i) {
        identical = identical && a.playRound(agent).reward == b.playRound(agent).reward;
    }

    CHECK(identical);
}

void testLongRunSingleDeckDoesNotCrash() {
    BlackjackGame game(Rules{.numDecks = 1, .penetration = 1.0}, 5);
    RandomAgent agent(5);

    bool ok = true;
    try {
        for (int i = 0; i < 200'000; ++i) {
            game.playRound(agent);
        }
    } catch (...) {
        ok = false;
    }

    CHECK(ok);
}

}

int main() {
    testPlayerBlackjackPaysThreeToTwo();
    testBothBlackjackIsPush();
    testDealerBlackjackLosesImmediately();
    testPlayerBustEndsRound();
    testStandReceivesTerminalReward();
    testDealerBust();
    testPush();
    testDealerSoft17Rule();
    testAutoStandOn21();
    testEuropeanPlayerActsBeforeDealerBlackjack();
    testEuropeanDealerBlackjackBeatsThreeCard21();
    testEuropeanBlackjackAgainstBlackjackIsPush();
    testEuropeanPlayerBlackjackPays();
    testEuropeanNormalRound();
    testEuropeanSameExpectedValue();
    testStepApiMatchesPlayRound();
    testActOutsidePlayerTurnThrows();
    testStepByStepHitThenStand();
    testSameSeedIsReproducible();
    testLongRunSingleDeckDoesNotCrash();

    return test::finish("test_game");
}
