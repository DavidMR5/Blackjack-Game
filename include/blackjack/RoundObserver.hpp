#pragma once

#include "Card.hpp"
#include "Hand.hpp"

namespace blackjack {

struct RoundResult;

class RoundObserver {
public:
    virtual ~RoundObserver() = default;

    virtual void onShuffle() {}
    virtual void onInitialDeal(
        const Hand& /*player*/,
        const Card& /*dealerUpCard*/,
        bool /*dealerHasHoleCard*/
    ) {}
    virtual void onPlayerCard(const Card& /*card*/, const Hand& /*player*/) {}
    virtual void onDealerReveal(const Hand& /*dealer*/) {}
    virtual void onDealerCard(const Card& /*card*/, const Hand& /*dealer*/) {}
    virtual void onRoundEnd(const RoundResult& /*result*/) {}
};

}
