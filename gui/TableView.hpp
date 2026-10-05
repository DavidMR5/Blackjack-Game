#pragma once

#include "CardAtlas.hpp"

#include "blackjack/BlackjackGame.hpp"
#include "blackjack/RoundObserver.hpp"

#include <raylib.h>

#include <deque>
#include <optional>
#include <string>
#include <vector>

namespace gui {

// Queues game events and plays them back as animations.
class TableView : public blackjack::RoundObserver {
public:
    TableView(const CardAtlas& atlas, const Font& font);

    void onShuffle() override;
    void onInitialDeal(const blackjack::Hand& player,
                       const blackjack::Card& dealerUpCard,
                       bool dealerHasHoleCard) override;
    void onPlayerCard(const blackjack::Card& card, const blackjack::Hand& player) override;
    void onDealerReveal(const blackjack::Hand& dealer) override;
    void onDealerCard(const blackjack::Card& card, const blackjack::Hand& dealer) override;
    void onRoundEnd(const blackjack::RoundResult& result) override;

    void update(float dt);
    void draw() const;

    bool isAnimating() const;

    std::optional<blackjack::RoundResult> takeFinishedResult();

    static constexpr Vector2 kShoePosition{1150.0f, 70.0f};

private:
    struct VisualCard {
        blackjack::Card card;
        Vector2 position;
        float flip;
        float flipTarget;
    };

    enum class StepType { DealPlayer, DealDealer, DealDealerHidden, RevealHole, Shuffle, Result };

    struct Step {
        StepType type;
        blackjack::Card card;
    };

    void enqueue(StepType type, blackjack::Card card = {blackjack::Suit::Spades, blackjack::Rank::Ace});
    void startStep(const Step& step);

    void drawHand(const std::vector<VisualCard>& cards, float y) const;
    void drawCard(const VisualCard& card) const;
    void drawTotal(const std::vector<VisualCard>& cards, float y, bool isDealer) const;
    void drawShoe() const;
    void drawBanner() const;

    static Vector2 slotPosition(std::size_t index, std::size_t count, float y);

    const CardAtlas& atlas_;
    const Font& font_;

    std::vector<VisualCard> player_;
    std::vector<VisualCard> dealer_;
    std::deque<Step> queue_;
    float stepTimer_ = 0.0f;

    bool holeHidden_ = false;
    bool holeRevealQueued_ = false;

    std::optional<blackjack::RoundResult> pendingResult_;
    std::optional<blackjack::RoundResult> shownResult_;
    bool resultTaken_ = true;
    float bannerTime_ = 0.0f;
    float shuffleToast_ = 0.0f;
};

}
