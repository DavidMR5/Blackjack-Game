#pragma once

#include "blackjack/Card.hpp"

#include <raylib.h>

namespace gui {

// All cards are drawn once at startup into one texture.
class CardAtlas {
public:
    static constexpr float kCardWidth = 110.0f;
    static constexpr float kCardHeight = 154.0f;

    explicit CardAtlas(const Font& font);
    ~CardAtlas();

    CardAtlas(const CardAtlas&) = delete;
    CardAtlas& operator=(const CardAtlas&) = delete;

    void drawFace(const blackjack::Card& card, Rectangle dest) const;
    void drawBack(Rectangle dest) const;

private:
    static constexpr int kScale = 2;
    static constexpr int kPadding = 4;
    static constexpr int kCellWidth = static_cast<int>(kCardWidth) * kScale + 2 * kPadding;
    static constexpr int kCellHeight = static_cast<int>(kCardHeight) * kScale + 2 * kPadding;

    Rectangle sourceRect(int column, int row) const;
    void drawCell(int column, int row, Rectangle dest) const;

    RenderTexture2D target_;
};

void drawSuit(blackjack::Suit suit, Vector2 center, float size, Color color, bool upsideDown = false);

}
