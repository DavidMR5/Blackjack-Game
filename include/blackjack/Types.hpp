#pragma once

#include <cstdint>
#include <string_view>

namespace blackjack {

enum class Suit : std::uint8_t {
    Hearts,
    Diamonds,
    Clubs,
    Spades
};

enum class Rank : std::uint8_t {
    Two = 2,
    Three,
    Four,
    Five,
    Six,
    Seven,
    Eight,
    Nine,
    Ten,
    Jack,
    Queen,
    King,
    Ace
};

enum class Action : std::uint8_t {
    Hit,
    Stand
};

enum class GameResult : std::uint8_t {
    PlayerBlackjack,
    PlayerWin,
    DealerWin,
    DealerBlackjack,
    Push
};

struct Rules {
    int numDecks = 6;
    bool dealerHitsSoft17 = false;
    double blackjackPayout = 1.5;
    double penetration = 0.75;

    // true: American rules (hole card checked first). false: European, no hole card.
    bool dealerPeeks = true;
};

// dealerUpCard: 2..11, 11 = Ace.
struct State {
    int playerSum{};
    int dealerUpCard{};
    bool usableAce{};

    friend bool operator==(const State&, const State&) = default;
};

constexpr std::string_view actionToString(Action action) {
    switch (action) {
        case Action::Hit:
            return "HIT";
        case Action::Stand:
            return "STAND";
    }

    return "UNKNOWN";
}

constexpr std::string_view resultToString(GameResult result) {
    switch (result) {
        case GameResult::PlayerBlackjack:
            return "BLACKJACK";
        case GameResult::PlayerWin:
            return "WIN";
        case GameResult::DealerWin:
            return "LOSS";
        case GameResult::DealerBlackjack:
            return "DEALER BLACKJACK";
        case GameResult::Push:
            return "PUSH";
    }

    return "UNKNOWN";
}

}
