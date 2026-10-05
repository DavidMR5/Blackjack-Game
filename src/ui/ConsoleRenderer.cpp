#include "ui/ConsoleRenderer.hpp"

#include "blackjack/BlackjackGame.hpp"

#include <ostream>

namespace blackjack {

ConsoleRenderer::ConsoleRenderer(std::ostream& out)
    : out_(out) {}

void ConsoleRenderer::printHand(const char* label, const Hand& hand) {
    out_ << "  " << label << ':';

    for (const Card& card : hand.getCards()) {
        out_ << ' ' << card.toShortString();
    }

    if (hand.isBlackjack()) {
        out_ << "  (BLACKJACK)\n";
    } else {
        out_ << "  (" << (hand.hasUsableAce() ? "soft " : "") << hand.getValue() << ")\n";
    }
}

void ConsoleRenderer::onShuffle() {
    out_ << "  * The shoe is reshuffled *\n";
}

void ConsoleRenderer::onInitialDeal(
    const Hand& player,
    const Card& dealerUpCard,
    bool dealerHasHoleCard
) {
    out_ << "  Dealer: " << dealerUpCard.toShortString()
         << (dealerHasHoleCard ? " ??" : "") << '\n';
    printHand("You   ", player);
}

void ConsoleRenderer::onPlayerCard(const Card&, const Hand& player) {
    printHand("You   ", player);
}

void ConsoleRenderer::onDealerReveal(const Hand& dealer) {
    printHand("Dealer", dealer);
}

void ConsoleRenderer::onDealerCard(const Card&, const Hand&) {
}

void ConsoleRenderer::onRoundEnd(const RoundResult& result) {
    out_ << "  >> " << resultToString(result.result);

    if (result.reward > 0) {
        out_ << "  (+" << result.reward << ")";
    } else if (result.reward < 0) {
        out_ << "  (" << result.reward << ")";
    }

    out_ << "\n\n";
}

}
