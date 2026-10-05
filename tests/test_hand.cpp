#include "TestFramework.hpp"

#include "blackjack/Hand.hpp"

using namespace blackjack;

namespace {

Hand makeHand(std::initializer_list<Rank> ranks) {
    Hand hand;

    for (Rank rank : ranks) {
        hand.addCard(Card(Suit::Spades, rank));
    }

    return hand;
}

void testSimpleHand() {
    const Hand hand = makeHand({Rank::Ten, Rank::Seven});

    CHECK(hand.getValue() == 17);
    CHECK(!hand.isBust());
    CHECK(!hand.hasUsableAce());
}

void testFaceCardsAreTen() {
    CHECK(makeHand({Rank::Jack, Rank::Queen}).getValue() == 20);
    CHECK(makeHand({Rank::King, Rank::Two}).getValue() == 12);
}

void testAceAdjustment() {
    const Hand hand = makeHand({Rank::Ace, Rank::King, Rank::Five});

    CHECK(hand.getValue() == 16);
    CHECK(!hand.hasUsableAce());
}

void testSoftHand() {
    const Hand hand = makeHand({Rank::Ace, Rank::Six});

    CHECK(hand.getValue() == 17);
    CHECK(hand.hasUsableAce());
}

void testTwoAces() {
    const Hand hand = makeHand({Rank::Ace, Rank::Ace});

    CHECK(hand.getValue() == 12);
    CHECK(hand.hasUsableAce());
    CHECK(!hand.isBlackjack());
}

void testManyAces() {
    const Hand hand = makeHand({Rank::Ace, Rank::Ace, Rank::Ace, Rank::Ace, Rank::Seven});

    CHECK(hand.getValue() == 21);
    CHECK(hand.hasUsableAce());
}

void testBlackjack() {
    CHECK(makeHand({Rank::Ace, Rank::King}).isBlackjack());
    CHECK(makeHand({Rank::Ten, Rank::Ace}).isBlackjack());
}

void testThreeCard21IsNotBlackjack() {
    const Hand hand = makeHand({Rank::Seven, Rank::Seven, Rank::Seven});

    CHECK(hand.getValue() == 21);
    CHECK(!hand.isBlackjack());
}

void testBust() {
    const Hand hand = makeHand({Rank::Ten, Rank::King, Rank::Five});

    CHECK(hand.isBust());
    CHECK(hand.getValue() == 25);
}

void testClear() {
    Hand hand = makeHand({Rank::Ten, Rank::Nine});
    hand.clear();

    CHECK(hand.size() == 0);
    CHECK(hand.getValue() == 0);
}

}

int main() {
    testSimpleHand();
    testFaceCardsAreTen();
    testAceAdjustment();
    testSoftHand();
    testTwoAces();
    testManyAces();
    testBlackjack();
    testThreeCard21IsNotBlackjack();
    testBust();
    testClear();

    return test::finish("test_hand");
}
