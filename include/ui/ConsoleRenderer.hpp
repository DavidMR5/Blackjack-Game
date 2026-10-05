#pragma once

#include "blackjack/RoundObserver.hpp"

#include <iosfwd>

namespace blackjack {

class ConsoleRenderer : public RoundObserver {
public:
    explicit ConsoleRenderer(std::ostream& out);

    void onShuffle() override;
    void onInitialDeal(const Hand& player, const Card& dealerUpCard, bool dealerHasHoleCard) override;
    void onPlayerCard(const Card& card, const Hand& player) override;
    void onDealerReveal(const Hand& dealer) override;
    void onDealerCard(const Card& card, const Hand& dealer) override;
    void onRoundEnd(const RoundResult& result) override;

private:
    void printHand(const char* label, const Hand& hand);

    std::ostream& out_;
};

}
