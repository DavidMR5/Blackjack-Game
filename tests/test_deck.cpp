#include "TestFramework.hpp"

#include "blackjack/Deck.hpp"

#include <map>
#include <utility>
#include <vector>

using namespace blackjack;

namespace {

std::vector<Card> drawAll(Deck& deck) {
    std::vector<Card> cards;

    while (!deck.empty()) {
        cards.push_back(deck.draw());
    }

    return cards;
}

void testSingleDeckHas52UniqueCards() {
    Deck deck(1, 42);
    deck.shuffle();

    CHECK(deck.size() == 52);

    std::map<std::pair<int, int>, int> counts;
    for (const Card& card : drawAll(deck)) {
        ++counts[{static_cast<int>(card.getSuit()), static_cast<int>(card.getRank())}];
    }

    CHECK(counts.size() == 52);
    for (const auto& [card, count] : counts) {
        CHECK(count == 1);
    }
}

void testShoeHasEveryCardNTimes() {
    Deck deck(6, 7);
    deck.shuffle();

    CHECK(deck.size() == 6 * 52);

    std::map<std::pair<int, int>, int> counts;
    for (const Card& card : drawAll(deck)) {
        ++counts[{static_cast<int>(card.getSuit()), static_cast<int>(card.getRank())}];
    }

    CHECK(counts.size() == 52);
    for (const auto& [card, count] : counts) {
        CHECK(count == 6);
    }
}

void testSameSeedSameOrder() {
    Deck a(2, 1234);
    Deck b(2, 1234);
    a.shuffle();
    b.shuffle();

    CHECK(drawAll(a) == drawAll(b));
}

void testDifferentSeedDifferentOrder() {
    Deck a(1, 1);
    Deck b(1, 2);
    a.shuffle();
    b.shuffle();

    CHECK(drawAll(a) != drawAll(b));
}

void testDrawFromEmptyThrows() {
    Deck deck(1, 0);
    drawAll(deck);

    CHECK(deck.empty());
    CHECK_THROWS(deck.draw());
}

void testResetRestoresAllCards() {
    Deck deck(1, 0);
    deck.draw();
    deck.draw();
    deck.reset();

    CHECK(deck.size() == 52);
}

void testPenetration() {
    Deck deck(1, 0);

    for (int i = 0; i < 38; ++i) {
        deck.draw();
    }
    CHECK(!deck.needsReshuffle(0.75));

    deck.draw();
    CHECK(deck.needsReshuffle(0.75));
}

void testStackedDeckDrawOrder() {
    const std::vector<Card> order = {
        Card(Suit::Hearts, Rank::Ace),
        Card(Suit::Clubs, Rank::Two),
        Card(Suit::Spades, Rank::King),
    };

    Deck deck = Deck::stacked(order);

    CHECK(drawAll(deck) == order);
}

}

int main() {
    testSingleDeckHas52UniqueCards();
    testShoeHasEveryCardNTimes();
    testSameSeedSameOrder();
    testDifferentSeedDifferentOrder();
    testDrawFromEmptyThrows();
    testResetRestoresAllCards();
    testPenetration();
    testStackedDeckDrawOrder();

    return test::finish("test_deck");
}
