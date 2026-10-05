#pragma once

#include "Card.hpp"

#include <cstddef>
#include <cstdint>
#include <random>
#include <vector>

namespace blackjack {

class Deck {
public:
    explicit Deck(int numDecks = 1, std::uint32_t seed = std::random_device{}());

    // Fixed draw order, for tests.
    static Deck stacked(const std::vector<Card>& drawOrder);

    void reset();
    void shuffle();

    Card draw();
    bool empty() const;
    std::size_t size() const;
    std::size_t fullSize() const;

    bool needsReshuffle(double penetration) const;

private:
    struct StackedTag {};
    Deck(StackedTag, const std::vector<Card>& drawOrder);

    int numDecks_;
    std::vector<Card> cards_;
    std::size_t fullSize_;
    std::mt19937 rng_;
};

}
