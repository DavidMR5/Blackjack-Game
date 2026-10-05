#include "blackjack/BlackjackGame.hpp"

#include "agents/BasicStrategyAgent.hpp"
#include "agents/HumanAgent.hpp"
#include "agents/MonteCarloAgent.hpp"
#include "agents/QLearningAgent.hpp"
#include "agents/RandomAgent.hpp"
#include "ui/ConsoleRenderer.hpp"

#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <optional>
#include <random>
#include <string>
#include <string_view>

using namespace blackjack;

namespace {

struct Options {
    bool play = false;
    int rounds = 1'000'000;
    int trainingRounds = 2'000'000;
    int monteCarloRounds = 100'000;
    int monteCarloSimulations = 500;
    std::uint32_t seed = std::random_device{}();
    Rules rules{};
    std::optional<bool> dealerPeeks;
};

void printUsage() {
    std::cout <<
        "Usage: blackjack [options]\n"
        "\n"
        "  --play            Play against the dealer yourself\n"
        "  --rounds N        Evaluation rounds per agent      (default 1000000)\n"
        "  --train N         Q-Learning training rounds       (default 2000000)\n"
        "  --mc-rounds N     Rounds for the Monte Carlo agent (default 100000)\n"
        "  --mc-sims N       Simulations per action for MC    (default 500)\n"
        "  --seed S          RNG seed, makes a run reproducible\n"
        "  --decks N         Decks in the shoe                (default 6)\n"
        "  --h17             Dealer hits soft 17              (default: stands)\n"
        "  --peek            American rules: dealer checks for blackjack first\n"
        "                    (default for the AI lab)\n"
        "  --no-peek         European rules: dealer takes the 2nd card after you play\n"
        "                    (default for --play)\n"
        "  --help            Show this message\n";
}

std::optional<Options> parseArgs(int argc, char** argv) {
    Options options;

    for (int i = 1; i < argc; ++i) {
        const std::string_view arg = argv[i];

        auto nextNumber = [&]() -> std::optional<long long> {
            if (i + 1 >= argc) {
                std::cerr << "Missing value for " << arg << '\n';
                return std::nullopt;
            }

            char* end = nullptr;
            const long long value = std::strtoll(argv[++i], &end, 10);

            if (*end != '\0' || value < 0) {
                std::cerr << "Invalid value for " << arg << ": " << argv[i] << '\n';
                return std::nullopt;
            }

            return value;
        };

        if (arg == "--help" || arg == "-h") {
            printUsage();
            std::exit(0);
        } else if (arg == "--play") {
            options.play = true;
        } else if (arg == "--h17") {
            options.rules.dealerHitsSoft17 = true;
        } else if (arg == "--peek") {
            options.dealerPeeks = true;
        } else if (arg == "--no-peek") {
            options.dealerPeeks = false;
        } else if (arg == "--rounds" || arg == "--train" || arg == "--mc-rounds" ||
                   arg == "--mc-sims" || arg == "--seed" || arg == "--decks") {
            const auto value = nextNumber();

            if (!value) {
                return std::nullopt;
            }

            if (arg == "--rounds") options.rounds = static_cast<int>(*value);
            if (arg == "--train") options.trainingRounds = static_cast<int>(*value);
            if (arg == "--mc-rounds") options.monteCarloRounds = static_cast<int>(*value);
            if (arg == "--mc-sims") options.monteCarloSimulations = static_cast<int>(*value);
            if (arg == "--seed") options.seed = static_cast<std::uint32_t>(*value);
            if (arg == "--decks") options.rules.numDecks = static_cast<int>(*value);
        } else {
            std::cerr << "Unknown option: " << arg << "\n\n";
            printUsage();
            return std::nullopt;
        }
    }

    if (options.rounds <= 0 || options.monteCarloRounds <= 0 ||
        options.monteCarloSimulations <= 0 || options.rules.numDecks <= 0) {
        std::cerr << "Rounds, simulations and decks must be greater than zero.\n";
        return std::nullopt;
    }

    options.rules.dealerPeeks = options.dealerPeeks.value_or(!options.play);

    return options;
}

// Welford's online mean/variance.
struct RunningStats {
    long long count = 0;
    double mean = 0.0;
    double m2 = 0.0;

    void add(double x) {
        ++count;
        const double delta = x - mean;
        mean += delta / static_cast<double>(count);
        m2 += delta * (x - mean);
    }

    double standardError() const {
        if (count < 2) {
            return 0.0;
        }

        const double variance = m2 / static_cast<double>(count - 1);
        return std::sqrt(variance / static_cast<double>(count));
    }
};

void runSimulation(const Options& options, Agent& agent, int rounds, std::string_view name) {
    BlackjackGame game(options.rules, options.seed);

    RunningStats stats;
    long long wins = 0;
    long long losses = 0;
    long long pushes = 0;

    const auto start = std::chrono::steady_clock::now();

    for (int i = 0; i < rounds; ++i) {
        const RoundResult round = game.playRound(agent);
        stats.add(round.reward);

        if (round.reward > 0) {
            ++wins;
        } else if (round.reward < 0) {
            ++losses;
        } else {
            ++pushes;
        }
    }

    const std::chrono::duration<double> elapsed = std::chrono::steady_clock::now() - start;
    const double percent = 100.0 / rounds;

    std::cout << std::fixed << std::setprecision(2)
              << std::left << std::setw(22) << name << std::right
              << std::setw(8) << static_cast<double>(wins) * percent << '%'
              << std::setw(8) << static_cast<double>(losses) * percent << '%'
              << std::setw(8) << static_cast<double>(pushes) * percent << '%'
              << std::setw(10) << std::showpos << stats.mean * 100.0 << '%' << std::noshowpos
              << "  +/- " << 1.96 * stats.standardError() * 100.0 << '%'
              << std::setw(12) << std::setprecision(0) << rounds / elapsed.count()
              << '\n';
}

double agreementWithBasicStrategy(const QLearningAgent& agent) {
    BasicStrategyAgent reference;
    int total = 0;
    int agree = 0;

    for (int soft = 0; soft <= 1; ++soft) {
        for (int sum = soft ? 13 : 12; sum <= 20; ++sum) {
            for (int dealer = 2; dealer <= 11; ++dealer) {
                const State state{sum, dealer, soft == 1};
                ++total;

                if (agent.getGreedyAction(state) == reference.chooseAction(state)) {
                    ++agree;
                }
            }
        }
    }

    return 100.0 * agree / total;
}

int runLab(const Options& options) {
    std::cout << "==============================================\n"
              << "               BLACKJACK AI LAB\n"
              << "==============================================\n"
              << "Rules: " << options.rules.numDecks << " decks, dealer "
              << (options.rules.dealerHitsSoft17 ? "hits" : "stands") << " on soft 17, "
              << "blackjack pays 3:2, "
              << (options.rules.dealerPeeks ? "dealer peeks" : "no hole card") << '\n'
              << "Seed:  " << options.seed << "  (re-run with --seed "
              << options.seed << " to reproduce)\n\n";

    std::cout << std::left << std::setw(22) << "Agent" << std::right
              << std::setw(9) << "Win" << std::setw(9) << "Loss" << std::setw(9) << "Push"
              << std::setw(11) << "EV/hand" << "  95% CI    "
              << std::setw(9) << "hands/s" << '\n'
              << std::string(84, '-') << '\n';

    RandomAgent randomAgent(options.seed);
    runSimulation(options, randomAgent, options.rounds, "Random");

    BasicStrategyAgent basicAgent;
    runSimulation(options, basicAgent, options.rounds, "Basic strategy");

    MonteCarloAgent monteCarloAgent(
        options.monteCarloSimulations,
        options.rules,
        options.seed
    );
    runSimulation(options, monteCarloAgent, options.monteCarloRounds, "Monte Carlo");

    QLearningAgent qAgent(0.001, 1.0, 0.3, options.seed);
    {
        BlackjackGame trainingGame(options.rules, options.seed + 1);

        for (int i = 0; i < options.trainingRounds; ++i) {
            trainingGame.playRound(qAgent);
        }
    }

    qAgent.setExplorationRate(0.0);
    runSimulation(options, qAgent, options.rounds, "Q-Learning (greedy)");

    std::cout << std::setprecision(1)
              << "\nQ-Learning trained for " << options.trainingRounds << " rounds; "
              << "its policy matches basic strategy in "
              << agreementWithBasicStrategy(qAgent) << "% of decisions.\n";

    qAgent.printPolicy(std::cout);

    return 0;
}

int runInteractive(const Options& options) {
    BlackjackGame game(options.rules, options.seed);
    BasicStrategyAgent advisor;
    HumanAgent human(std::cin, std::cout, &advisor);
    ConsoleRenderer renderer(std::cout);

    double bankroll = 0.0;
    int hands = 0;

    std::cout << "Blackjack - " << options.rules.numDecks << " decks, blackjack pays 3:2, "
              << (options.rules.dealerPeeks
                      ? "dealer checks for blackjack first.\n"
                      : "European rules (dealer draws the 2nd card after you).\n")
              << "Type ? during a hand for a basic strategy hint.\n\n";

    while (!human.wantsToQuit()) {
        std::cout << "--- Hand " << (hands + 1) << " ---\n";

        const RoundResult round = game.playRound(human, &renderer);
        bankroll += round.reward;
        ++hands;

        std::cout << std::showpos << "  Balance: " << bankroll << " units"
                  << std::noshowpos << "\n\n";
    }

    std::cout << "You played " << hands << " hands. Final balance: "
              << std::showpos << bankroll << std::noshowpos << " units.\n";

    return 0;
}

}

int main(int argc, char** argv) {
    const std::optional<Options> options = parseArgs(argc, argv);

    if (!options) {
        return 1;
    }

    return options->play ? runInteractive(*options) : runLab(*options);
}
