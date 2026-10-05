#include "blackjack/Deck.hpp"

#include <algorithm>
#include <array>
#include <stdexcept>

namespace blackjack {

namespace {

constexpr std::array<Suit, 4> kSuits = {
    Suit::Hearts,
    Suit::Diamonds,
    Suit::Clubs,
    Suit::Spades
};

constexpr std::size_t kCardsPerDeck = 52;

}

Deck::Deck(int numDecks, std::uint32_t seed)
    : numDecks_(std::max(1, numDecks)),
      fullSize_(static_cast<std::size_t>(numDecks_) * kCardsPerDeck),
      rng_(seed) {
    reset();
}

Deck::Deck(StackedTag, const std::vector<Card>& drawOrder)
    : numDecks_(1),
      cards_(drawOrder.rbegin(), drawOrder.rend()),
      fullSize_(drawOrder.size()),
      rng_(0) {}

Deck Deck::stacked(const std::vector<Card>& drawOrder) {
    return Deck(StackedTag{}, drawOrder);
}

void Deck::reset() {
    cards_.clear();
    cards_.reserve(static_cast<std::size_t>(numDecks_) * kCardsPerDeck);

    for (int deck = 0; deck < numDecks_; ++deck) {
        for (Suit suit : kSuits) {
            for (int rank = static_cast<int>(Rank::Two);
                 rank <= static_cast<int>(Rank::Ace);
                 ++rank) {
                cards_.emplace_back(suit, static_cast<Rank>(rank));
            }
        }
    }

    fullSize_ = cards_.size();
}

void Deck::shuffle() {
    std::shuffle(cards_.begin(), cards_.end(), rng_);
}

Card Deck::draw() {
    if (cards_.empty()) {
        throw std::runtime_error("Cannot draw from an empty deck.");
    }

    const Card card = cards_.back();
    cards_.pop_back();

    return card;
}

bool Deck::empty() const {
    return cards_.empty();
}

std::size_t Deck::size() const {
    return cards_.size();
}

std::size_t Deck::fullSize() const {
    return fullSize_;
}

bool Deck::needsReshuffle(double penetration) const {
    const double dealt = static_cast<double>(fullSize_ - cards_.size());
    return dealt >= penetration * static_cast<double>(fullSize_);
}

}
