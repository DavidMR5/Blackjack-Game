#include "blackjack/Card.hpp"

namespace blackjack {

namespace {

std::string_view rankToString(Rank rank) {
    switch (rank) {
        case Rank::Two: return "2";
        case Rank::Three: return "3";
        case Rank::Four: return "4";
        case Rank::Five: return "5";
        case Rank::Six: return "6";
        case Rank::Seven: return "7";
        case Rank::Eight: return "8";
        case Rank::Nine: return "9";
        case Rank::Ten: return "10";
        case Rank::Jack: return "J";
        case Rank::Queen: return "Q";
        case Rank::King: return "K";
        case Rank::Ace: return "A";
    }

    return "?";
}

std::string_view suitToString(Suit suit) {
    switch (suit) {
        case Suit::Hearts: return "Hearts";
        case Suit::Diamonds: return "Diamonds";
        case Suit::Clubs: return "Clubs";
        case Suit::Spades: return "Spades";
    }

    return "?";
}

}

std::string Card::toString() const {
    std::string result(rankToString(rank_));
    result += " of ";
    result += suitToString(suit_);
    return result;
}

std::string Card::toShortString() const {
    std::string result(rankToString(rank_));
    result += static_cast<char>(suitToString(suit_).front() - 'A' + 'a');
    return result;
}

}
