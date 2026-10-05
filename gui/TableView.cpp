#include "TableView.hpp"

#include "Theme.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace gui {

using blackjack::Card;
using blackjack::GameResult;
using blackjack::Hand;
using blackjack::RoundResult;

namespace {

constexpr float kDealerRowY = 150.0f;
constexpr float kPlayerRowY = 430.0f;
constexpr float kCardSpacing = 72.0f;

constexpr float kDealDelay = 0.30f;
constexpr float kRevealDelay = 0.45f;
constexpr float kShuffleDelay = 0.70f;
constexpr float kFlipDuration = 0.28f;

constexpr float kPi = 3.14159265f;

}

TableView::TableView(const CardAtlas& atlas, const Font& font)
    : atlas_(atlas), font_(font) {}

void TableView::enqueue(StepType type, Card card) {
    queue_.push_back({type, card});
}

void TableView::onShuffle() {
    enqueue(StepType::Shuffle);
}

void TableView::onInitialDeal(const Hand& player, const Card& dealerUpCard, bool dealerHasHoleCard) {
    player_.clear();
    dealer_.clear();
    shownResult_.reset();
    pendingResult_.reset();
    resultTaken_ = true;
    holeHidden_ = dealerHasHoleCard;
    holeRevealQueued_ = false;

    enqueue(StepType::DealPlayer, player.getCards()[0]);
    enqueue(StepType::DealDealer, dealerUpCard);
    enqueue(StepType::DealPlayer, player.getCards()[1]);

    if (dealerHasHoleCard) {
        enqueue(StepType::DealDealerHidden);
    }
}

void TableView::onPlayerCard(const Card& card, const Hand&) {
    enqueue(StepType::DealPlayer, card);
}

void TableView::onDealerReveal(const Hand& dealer) {
    if (holeHidden_ && !holeRevealQueued_ && dealer.size() >= 2) {
        enqueue(StepType::RevealHole, dealer.getCards()[1]);
        holeRevealQueued_ = true;
    }
}

void TableView::onDealerCard(const Card& card, const Hand& dealer) {
    onDealerReveal(dealer);
    enqueue(StepType::DealDealer, card);
}

void TableView::onRoundEnd(const RoundResult& result) {
    pendingResult_ = result;
    enqueue(StepType::Result);
}

void TableView::startStep(const Step& step) {
    switch (step.type) {
        case StepType::DealPlayer:
            player_.push_back({step.card, kShoePosition, 0.0f, 1.0f});
            stepTimer_ += kDealDelay;
            break;

        case StepType::DealDealer:
            dealer_.push_back({step.card, kShoePosition, 0.0f, 1.0f});
            stepTimer_ += kDealDelay;
            break;

        case StepType::DealDealerHidden:
            dealer_.push_back({step.card, kShoePosition, 0.0f, 0.0f});
            stepTimer_ += kDealDelay;
            break;

        case StepType::RevealHole:
            if (dealer_.size() >= 2) {
                dealer_[1].card = step.card;
                dealer_[1].flipTarget = 1.0f;
            }
            holeHidden_ = false;
            stepTimer_ += kRevealDelay;
            break;

        case StepType::Shuffle:
            shuffleToast_ = 1.8f;
            stepTimer_ += kShuffleDelay;
            break;

        case StepType::Result:
            shownResult_ = pendingResult_;
            resultTaken_ = false;
            bannerTime_ = 0.0f;
            break;
    }
}

void TableView::update(float dt) {
    stepTimer_ -= dt;

    while (stepTimer_ <= 0.0f && !queue_.empty()) {
        const Step step = queue_.front();
        queue_.pop_front();
        startStep(step);
    }

    if (queue_.empty()) {
        stepTimer_ = std::max(stepTimer_, 0.0f);
    }

    const float follow = 1.0f - std::exp(-dt * 12.0f);

    auto animate = [&](std::vector<VisualCard>& cards, float rowY) {
        for (std::size_t i = 0; i < cards.size(); ++i) {
            VisualCard& card = cards[i];
            const Vector2 target = slotPosition(i, cards.size(), rowY);

            card.position.x += (target.x - card.position.x) * follow;
            card.position.y += (target.y - card.position.y) * follow;

            const float step = dt / kFlipDuration;
            if (card.flip < card.flipTarget) {
                card.flip = std::min(card.flip + step, card.flipTarget);
            } else if (card.flip > card.flipTarget) {
                card.flip = std::max(card.flip - step, card.flipTarget);
            }
        }
    };

    animate(player_, kPlayerRowY);
    animate(dealer_, kDealerRowY);

    bannerTime_ += dt;
    shuffleToast_ = std::max(0.0f, shuffleToast_ - dt);
}

bool TableView::isAnimating() const {
    return !queue_.empty() || stepTimer_ > 0.0f;
}

std::optional<RoundResult> TableView::takeFinishedResult() {
    if (shownResult_ && !resultTaken_) {
        resultTaken_ = true;
        return shownResult_;
    }
    return std::nullopt;
}

Vector2 TableView::slotPosition(std::size_t index, std::size_t count, float y) {
    const float handWidth = static_cast<float>(count - 1) * kCardSpacing + CardAtlas::kCardWidth;
    const float startX = theme::kScreenWidth * 0.5f - handWidth * 0.5f;
    return {startX + static_cast<float>(index) * kCardSpacing, y};
}

void TableView::drawCard(const VisualCard& card) const {
    // Flip: shrink to zero width, swap side, grow back.
    const float widthScale = std::fabs(std::cos(kPi * card.flip));
    const float lift = std::sin(kPi * card.flip) * 10.0f;
    const float width = CardAtlas::kCardWidth * widthScale;

    const Rectangle dest{
        card.position.x + (CardAtlas::kCardWidth - width) * 0.5f,
        card.position.y - lift,
        width,
        CardAtlas::kCardHeight
    };

    DrawRectangleRounded({dest.x + 3.0f, dest.y + 6.0f + lift, dest.width, dest.height},
                         0.12f, 8, ColorAlpha(BLACK, 0.35f));

    if (card.flip >= 0.5f) {
        atlas_.drawFace(card.card, dest);
    } else {
        atlas_.drawBack(dest);
    }
}

void TableView::drawHand(const std::vector<VisualCard>& cards, float) const {
    for (const VisualCard& card : cards) {
        drawCard(card);
    }
}

void TableView::drawTotal(const std::vector<VisualCard>& cards, float y, bool isDealer) const {
    Hand visible;
    for (const VisualCard& card : cards) {
        if (card.flipTarget >= 1.0f && card.flip >= 0.5f) {
            visible.addCard(card.card);
        }
    }

    if (visible.size() == 0) {
        return;
    }

    std::string text;
    Color fill = theme::kPill;
    Color ink = theme::kText;

    if (visible.isBlackjack() && visible.size() == cards.size()) {
        text = "BLACKJACK";
        fill = theme::kGold;
        ink = theme::kInkDark;
    } else if (visible.isBust()) {
        text = "BUST " + std::to_string(visible.getValue());
        fill = theme::kLoss;
    } else if (visible.hasUsableAce()) {
        text = "soft " + std::to_string(visible.getValue());
    } else {
        text = std::to_string(visible.getValue());
    }

    const float pillY = isDealer ? y - 46.0f : y + CardAtlas::kCardHeight + 16.0f;
    const char* who = isDealer ? "DEALER" : "YOU";

    const float size = 24.0f;
    const Vector2 textSize = MeasureTextEx(font_, text.c_str(), size, 1.0f);
    const Vector2 whoSize = MeasureTextEx(font_, who, 16.0f, 2.0f);
    const float width = textSize.x + whoSize.x + 44.0f;
    const Rectangle pill{theme::kScreenWidth * 0.5f - width * 0.5f, pillY, width, 34.0f};

    DrawRectangleRounded(pill, 1.0f, 16, fill);
    DrawTextEx(font_, who, {pill.x + 16.0f, pill.y + 9.0f}, 16.0f, 2.0f, ColorAlpha(ink, 0.7f));
    DrawTextEx(font_, text.c_str(), {pill.x + 28.0f + whoSize.x, pill.y + 5.0f}, size, 1.0f, ink);
}

void TableView::drawShoe() const {
    const Rectangle box{kShoePosition.x - 14.0f, kShoePosition.y - 14.0f,
                        CardAtlas::kCardWidth + 28.0f, CardAtlas::kCardHeight + 28.0f};
    DrawRectangleRounded({box.x + 4.0f, box.y + 8.0f, box.width, box.height}, 0.12f, 8,
                         ColorAlpha(BLACK, 0.35f));
    DrawRectangleRounded(box, 0.12f, 8, theme::kShoe);
    DrawRectangleRoundedLinesEx(box, 0.12f, 8, 2.0f, ColorAlpha(theme::kGold, 0.6f));

    for (int i = 3; i >= 0; --i) {
        const float offset = static_cast<float>(i) * 2.0f;
        atlas_.drawBack({kShoePosition.x - offset, kShoePosition.y - offset,
                         CardAtlas::kCardWidth, CardAtlas::kCardHeight});
    }
}

void TableView::drawBanner() const {
    if (!shownResult_) {
        return;
    }

    const RoundResult& result = *shownResult_;
    const bool playerBust = result.player.isBust();

    std::string title;
    Color color = theme::kText;

    switch (result.result) {
        case GameResult::PlayerBlackjack:
            title = "BLACKJACK!";
            color = theme::kGold;
            break;
        case GameResult::PlayerWin:
            title = result.dealer.isBust() ? "DEALER BUSTS - YOU WIN" : "YOU WIN";
            color = theme::kWin;
            break;
        case GameResult::DealerWin:
            title = playerBust ? "BUST" : "DEALER WINS";
            color = theme::kLoss;
            break;
        case GameResult::DealerBlackjack:
            title = "DEALER BLACKJACK";
            color = theme::kLoss;
            break;
        case GameResult::Push:
            title = "PUSH";
            color = theme::kText;
            break;
    }

    char amount[32];
    if (result.reward == 0.0) {
        std::snprintf(amount, sizeof(amount), "bet returned");
    } else {
        const bool plural = std::fabs(result.reward) != 1.0;
        std::snprintf(amount, sizeof(amount), "%+g unit%s", result.reward, plural ? "s" : "");
    }

    const float t = std::min(bannerTime_ / 0.3f, 1.0f);
    const float c1 = 1.70158f;
    const float ease = 1.0f + (c1 + 1.0f) * std::pow(t - 1.0f, 3.0f) + c1 * std::pow(t - 1.0f, 2.0f);
    const float scale = 0.6f + 0.4f * ease;

    const float titleSize = 46.0f * scale;
    const Vector2 titleDim = MeasureTextEx(font_, title.c_str(), titleSize, 2.0f);
    const float width = std::max(titleDim.x + 120.0f, 320.0f * scale);
    const float height = 96.0f * scale;
    const Vector2 center{theme::kScreenWidth * 0.5f, 367.0f};
    const Rectangle box{center.x - width * 0.5f, center.y - height * 0.5f, width, height};

    DrawRectangleRounded(box, 0.35f, 16, ColorAlpha(theme::kBanner, 0.88f * t));
    DrawRectangleRoundedLinesEx(box, 0.35f, 16, 2.0f, ColorAlpha(color, t));
    DrawTextEx(font_, title.c_str(),
               {center.x - titleDim.x * 0.5f, center.y - titleDim.y * 0.5f - 12.0f * scale},
               titleSize, 2.0f, ColorAlpha(color, t));

    const float amountSize = 24.0f * scale;
    const Vector2 amountDim = MeasureTextEx(font_, amount, amountSize, 1.0f);
    DrawTextEx(font_, amount,
               {center.x - amountDim.x * 0.5f, center.y + 18.0f * scale},
               amountSize, 1.0f, ColorAlpha(theme::kText, 0.85f * t));
}

void TableView::draw() const {
    drawShoe();

    drawHand(dealer_, kDealerRowY);
    drawHand(player_, kPlayerRowY);

    drawTotal(dealer_, kDealerRowY, true);
    drawTotal(player_, kPlayerRowY, false);

    drawBanner();

    if (shuffleToast_ > 0.0f) {
        const float alpha = std::min(shuffleToast_ / 0.4f, 1.0f);
        const char* text = "Shuffling the shoe...";
        const Vector2 size = MeasureTextEx(font_, text, 22.0f, 1.0f);
        DrawTextEx(font_, text,
                   {kShoePosition.x + CardAtlas::kCardWidth * 0.5f - size.x * 0.5f,
                    kShoePosition.y + CardAtlas::kCardHeight + 26.0f},
                   22.0f, 1.0f, ColorAlpha(theme::kGold, alpha));
    }
}

}
