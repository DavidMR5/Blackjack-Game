#include "blackjack/Hand.hpp"

namespace blackjack {

void Hand::addCard(const Card& card) {
    cards_.push_back(card);
}

void Hand::clear() {
    cards_.clear();
}

Hand::Totals Hand::computeTotals() const {
    int value = 0;
    int acesAsEleven = 0;

    for (const Card& card : cards_) {
        value += card.getValue();

        if (card.getRank() == Rank::Ace) {
            ++acesAsEleven;
        }
    }

    // Count Aces as 1 until the hand is not bust.
    while (value > 21 && acesAsEleven > 0) {
        value -= 10;
        --acesAsEleven;
    }

    return Totals{value, acesAsEleven > 0};
}

int Hand::getValue() const {
    return computeTotals().value;
}

bool Hand::isBust() const {
    return getValue() > 21;
}

bool Hand::isBlackjack() const {
    return cards_.size() == 2 && getValue() == 21;
}

bool Hand::hasUsableAce() const {
    return computeTotals().soft;
}

std::size_t Hand::size() const {
    return cards_.size();
}

const std::vector<Card>& Hand::getCards() const {
    return cards_;
}

}
