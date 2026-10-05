#pragma once

#include "Card.hpp"

#include <cstddef>
#include <vector>

namespace blackjack {

class Hand {
public:
    void addCard(const Card& card);
    void clear();

    int getValue() const;
    bool isBust() const;
    bool isBlackjack() const;

    bool hasUsableAce() const;

    std::size_t size() const;
    const std::vector<Card>& getCards() const;

private:
    struct Totals {
        int value;
        bool soft;
    };

    Totals computeTotals() const;

    std::vector<Card> cards_;
};

}
