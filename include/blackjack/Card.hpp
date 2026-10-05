#pragma once

#include "Types.hpp"

#include <string>

namespace blackjack {

class Card {
public:
    constexpr Card(Suit suit, Rank rank)
        : suit_(suit), rank_(rank) {}

    constexpr Suit getSuit() const { return suit_; }
    constexpr Rank getRank() const { return rank_; }

    constexpr int getValue() const {
        if (rank_ == Rank::Ace) {
            return 11;
        }

        if (rank_ >= Rank::Ten) {
            return 10;
        }

        return static_cast<int>(rank_);
    }

    std::string toString() const;
    std::string toShortString() const;

    friend constexpr bool operator==(const Card&, const Card&) = default;

private:
    Suit suit_;
    Rank rank_;
};

}
