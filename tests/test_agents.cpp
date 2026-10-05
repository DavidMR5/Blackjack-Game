#include "TestFramework.hpp"

#include "agents/BasicStrategyAgent.hpp"
#include "agents/HumanAgent.hpp"
#include "agents/MonteCarloAgent.hpp"
#include "agents/QLearningAgent.hpp"
#include "blackjack/BlackjackGame.hpp"

#include <sstream>

using namespace blackjack;

namespace {

constexpr int kAce = 11;

void testBasicStrategyHardTotals() {
    BasicStrategyAgent agent;

    CHECK(agent.chooseAction({11, kAce, false}) == Action::Hit);
    CHECK(agent.chooseAction({12, 3, false}) == Action::Hit);
    CHECK(agent.chooseAction({12, 4, false}) == Action::Stand);
    CHECK(agent.chooseAction({16, 6, false}) == Action::Stand);
    CHECK(agent.chooseAction({16, 10, false}) == Action::Hit);
    CHECK(agent.chooseAction({16, kAce, false}) == Action::Hit);
    CHECK(agent.chooseAction({17, kAce, false}) == Action::Stand);
}

void testBasicStrategySoftTotals() {
    BasicStrategyAgent agent;

    CHECK(agent.chooseAction({17, 6, true}) == Action::Hit);
    CHECK(agent.chooseAction({18, 7, true}) == Action::Stand);
    CHECK(agent.chooseAction({18, 9, true}) == Action::Hit);
    CHECK(agent.chooseAction({18, kAce, true}) == Action::Hit);
    CHECK(agent.chooseAction({19, 10, true}) == Action::Stand);
}

void testMonteCarloClearDecisions() {
    MonteCarloAgent agent(2000, Rules{}, 42);

    CHECK(agent.chooseAction({8, 6, false}) == Action::Hit);
    CHECK(agent.chooseAction({11, 10, false}) == Action::Hit);
    CHECK(agent.chooseAction({20, 10, false}) == Action::Stand);
    CHECK(agent.chooseAction({19, kAce, false}) == Action::Stand);
}

void testMonteCarloValueEstimates() {
    MonteCarloAgent agent(20'000, Rules{}, 7);

    CHECK(agent.estimateValue({20, 6, false}, Action::Stand) > 0.5);

    CHECK(agent.estimateValue({12, 10, false}, Action::Stand) < -0.4);
}

void testQLearningUpdate() {
    QLearningAgent agent(0.0, 1.0, 0.0, 1);
    const State state{20, 10, false};

    agent.learn(state, Action::Stand, 1.0, state, true);
    CHECK(agent.getQValue(state, Action::Stand) == 1.0);

    agent.learn(state, Action::Stand, -1.0, state, true);
    CHECK(agent.getQValue(state, Action::Stand) == 0.0);

    CHECK(agent.getGreedyAction(state) == Action::Stand);
}

void testQLearningStatesAreIndependent() {
    QLearningAgent agent(0.0, 1.0, 0.0, 1);

    agent.learn({16, 10, false}, Action::Hit, 1.0, {}, true);

    CHECK(agent.getQValue({16, 10, true}, Action::Hit) == 0.0);
    CHECK(agent.getQValue({16, 11, false}, Action::Hit) == 0.0);
    CHECK(agent.getQValue({17, 10, false}, Action::Hit) == 0.0);
}

void testQLearningLearnsObviousPolicy() {
    QLearningAgent agent(0.005, 1.0, 0.2, 3);
    BlackjackGame game(Rules{}, 3);

    for (int i = 0; i < 300'000; ++i) {
        game.playRound(agent);
    }

    CHECK(agent.getGreedyAction({20, 10, false}) == Action::Stand);
    CHECK(agent.getGreedyAction({19, 6, false}) == Action::Stand);
    CHECK(agent.getGreedyAction({8, 10, false}) == Action::Hit);
    CHECK(agent.getGreedyAction({13, 10, true}) == Action::Hit);
}

void testHumanAgentInput() {
    {
        std::istringstream in("x\nh\n");
        std::ostringstream out;
        HumanAgent human(in, out);

        CHECK(human.chooseAction({12, 10, false}) == Action::Hit);
        CHECK(out.str().find("Please type") != std::string::npos);
    }

    {
        BasicStrategyAgent advisor;
        std::istringstream in("?\nS\n");
        std::ostringstream out;
        HumanAgent human(in, out, &advisor);

        CHECK(human.chooseAction({16, 10, false}) == Action::Stand);
        CHECK(out.str().find("Hint: HIT") != std::string::npos);
    }

    {
        std::istringstream in("");
        std::ostringstream out;
        HumanAgent human(in, out);

        CHECK(human.chooseAction({12, 10, false}) == Action::Stand);
        CHECK(human.wantsToQuit());
    }
}

}

int main() {
    testBasicStrategyHardTotals();
    testBasicStrategySoftTotals();
    testMonteCarloClearDecisions();
    testMonteCarloValueEstimates();
    testQLearningUpdate();
    testQLearningStatesAreIndependent();
    testQLearningLearnsObviousPolicy();
    testHumanAgentInput();

    return test::finish("test_agents");
}
