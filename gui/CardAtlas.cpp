#include "CardAtlas.hpp"

#include <string>

namespace gui {

using blackjack::Card;
using blackjack::Rank;
using blackjack::Suit;

namespace {

constexpr Color kPaper{250, 247, 238, 255};
constexpr Color kPaperEdge{205, 200, 188, 255};
constexpr Color kRed{196, 30, 48, 255};
constexpr Color kBlack{28, 28, 38, 255};
constexpr Color kBackRed{142, 22, 38, 255};
constexpr Color kBackPattern{170, 40, 58, 255};
constexpr Color kGold{214, 175, 92, 255};

constexpr int kRanks = 13;
constexpr int kBackRow = 4;
constexpr float kArtScale = 2.0f;

void drawTriangle(Vector2 a, Vector2 b, Vector2 c, Color color) {
    const float cross = (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);

    if (cross < 0.0f) {
        DrawTriangle(a, b, c, color);
    } else {
        DrawTriangle(a, c, b, color);
    }
}

const char* rankLabel(Rank rank) {
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

bool isRed(Suit suit) {
    return suit == Suit::Hearts || suit == Suit::Diamonds;
}

void drawFaceArt(const Font& font, const Card& card, Rectangle r) {
    const float scale = kArtScale;
    const Color ink = isRed(card.getSuit()) ? kRed : kBlack;
    const char* label = rankLabel(card.getRank());

    DrawRectangleRounded(r, 0.12f, 16, kPaper);
    DrawRectangleRoundedLinesEx(r, 0.12f, 16, 2.0f * scale, kPaperEdge);

    const float indexSize = 30.0f * scale;
    const float spacing = 0.0f;
    const Vector2 labelSize = MeasureTextEx(font, label, indexSize, spacing);
    const float indexX = r.x + 9.0f * scale;
    const float indexY = r.y + 6.0f * scale;

    DrawTextEx(font, label, {indexX, indexY}, indexSize, spacing, ink);
    drawSuit(card.getSuit(),
             {indexX + labelSize.x * 0.5f, indexY + labelSize.y + 9.0f * scale},
             15.0f * scale, ink);

    DrawTextPro(font, label,
                {r.x + r.width - 9.0f * scale, r.y + r.height - 6.0f * scale},
                {0.0f, 0.0f}, 180.0f, indexSize, spacing, ink);
    drawSuit(card.getSuit(),
             {r.x + r.width - 9.0f * scale - labelSize.x * 0.5f,
              r.y + r.height - 6.0f * scale - labelSize.y - 9.0f * scale},
             15.0f * scale, ink, true);

    const Vector2 center{r.x + r.width * 0.5f, r.y + r.height * 0.5f};
    const Rank rank = card.getRank();

    if (rank == Rank::Jack || rank == Rank::Queen || rank == Rank::King) {
        const Rectangle panel{r.x + 26.0f * scale, r.y + 30.0f * scale,
                              r.width - 52.0f * scale, r.height - 60.0f * scale};
        DrawRectangleRounded(panel, 0.08f, 8, ColorAlpha(ink, 0.08f));
        DrawRectangleRoundedLinesEx(panel, 0.08f, 8, 1.5f * scale, ColorAlpha(kGold, 0.9f));

        const float letterSize = 54.0f * scale;
        const Vector2 letter = MeasureTextEx(font, label, letterSize, spacing);
        DrawTextEx(font, label,
                   {center.x - letter.x * 0.5f, center.y - letter.y * 0.5f - 12.0f * scale},
                   letterSize, spacing, ink);
        drawSuit(card.getSuit(), {center.x, center.y + 30.0f * scale}, 20.0f * scale, ink);
    } else {
        const float pipSize = (rank == Rank::Ace ? 64.0f : 46.0f) * scale;
        drawSuit(card.getSuit(), center, pipSize, ink);
    }
}

void drawBackArt(Rectangle r) {
    const float scale = kArtScale;

    DrawRectangleRounded(r, 0.12f, 16, kPaper);
    const Rectangle inner{r.x + 7.0f * scale, r.y + 7.0f * scale,
                          r.width - 14.0f * scale, r.height - 14.0f * scale};
    DrawRectangleRounded(inner, 0.10f, 16, kBackRed);

    const float step = 12.0f * scale;
    const float radius = 4.5f * scale;
    for (float y = inner.y + step * 0.75f; y < inner.y + inner.height - radius; y += step) {
        for (float x = inner.x + step * 0.75f; x < inner.x + inner.width - radius; x += step) {
            DrawPoly({x, y}, 4, radius, 0.0f, kBackPattern);
        }
    }

    DrawRectangleRoundedLinesEx(inner, 0.10f, 16, 1.5f * scale, kGold);
    DrawCircleV({r.x + r.width * 0.5f, r.y + r.height * 0.5f}, 15.0f * scale, kBackRed);
    DrawCircleLinesV({r.x + r.width * 0.5f, r.y + r.height * 0.5f}, 15.0f * scale, kGold);
    drawSuit(Suit::Spades, {r.x + r.width * 0.5f, r.y + r.height * 0.5f}, 16.0f * scale, kGold);
}

}

void drawSuit(Suit suit, Vector2 center, float size, Color color, bool upsideDown) {
    const float flip = upsideDown ? -1.0f : 1.0f;
    auto p = [&](float x, float y) {
        return Vector2{center.x + x * size, center.y + y * size * flip};
    };

    switch (suit) {
        case Suit::Hearts:
            DrawCircleV(p(-0.24f, -0.14f), 0.27f * size, color);
            DrawCircleV(p(0.24f, -0.14f), 0.27f * size, color);
            drawTriangle(p(-0.495f, -0.04f), p(0.495f, -0.04f), p(0.0f, 0.48f), color);
            break;

        case Suit::Diamonds:
            drawTriangle(p(0.0f, -0.5f), p(-0.36f, 0.0f), p(0.36f, 0.0f), color);
            drawTriangle(p(0.0f, 0.5f), p(-0.36f, 0.0f), p(0.36f, 0.0f), color);
            break;

        case Suit::Spades:
            DrawCircleV(p(-0.23f, 0.10f), 0.25f * size, color);
            DrawCircleV(p(0.23f, 0.10f), 0.25f * size, color);
            drawTriangle(p(-0.47f, 0.02f), p(0.47f, 0.02f), p(0.0f, -0.5f), color);
            drawTriangle(p(0.0f, 0.12f), p(-0.16f, 0.5f), p(0.16f, 0.5f), color);
            break;

        case Suit::Clubs:
            DrawCircleV(p(0.0f, -0.24f), 0.22f * size, color);
            DrawCircleV(p(-0.24f, 0.08f), 0.22f * size, color);
            DrawCircleV(p(0.24f, 0.08f), 0.22f * size, color);
            DrawCircleV(p(0.0f, 0.02f), 0.12f * size, color);
            drawTriangle(p(0.0f, 0.0f), p(-0.16f, 0.5f), p(0.16f, 0.5f), color);
            break;
    }
}

CardAtlas::CardAtlas(const Font& font)
    : target_(LoadRenderTexture(kCellWidth * kRanks, kCellHeight * (kBackRow + 1))) {
    const float width = kCardWidth * kScale;
    const float height = kCardHeight * kScale;

    BeginTextureMode(target_);
    ClearBackground(BLANK);

    const Suit suits[] = {Suit::Hearts, Suit::Diamonds, Suit::Clubs, Suit::Spades};

    for (int row = 0; row < 4; ++row) {
        for (int column = 0; column < kRanks; ++column) {
            const Card card(suits[row], static_cast<Rank>(column + static_cast<int>(Rank::Two)));
            const Rectangle cell{
                static_cast<float>(column * kCellWidth + kPadding),
                static_cast<float>(row * kCellHeight + kPadding),
                width, height
            };
            drawFaceArt(font, card, cell);
        }
    }

    drawBackArt({static_cast<float>(kPadding),
                 static_cast<float>(kBackRow * kCellHeight + kPadding),
                 width, height});

    EndTextureMode();

    GenTextureMipmaps(&target_.texture);
    SetTextureFilter(target_.texture, TEXTURE_FILTER_TRILINEAR);
}

CardAtlas::~CardAtlas() {
    UnloadRenderTexture(target_);
}

Rectangle CardAtlas::sourceRect(int column, int row) const {
    const float width = kCardWidth * kScale;
    const float height = kCardHeight * kScale;
    const float x = static_cast<float>(column * kCellWidth + kPadding);
    const float y = static_cast<float>(row * kCellHeight + kPadding);
    const float textureHeight = static_cast<float>(target_.texture.height);

    // Render textures are stored upside down.
    return {x, textureHeight - y - height, width, -height};
}

void CardAtlas::drawCell(int column, int row, Rectangle dest) const {
    DrawTexturePro(target_.texture, sourceRect(column, row), dest, {0.0f, 0.0f}, 0.0f, WHITE);
}

void CardAtlas::drawFace(const Card& card, Rectangle dest) const {
    const int row = static_cast<int>(card.getSuit());
    const int column = static_cast<int>(card.getRank()) - static_cast<int>(Rank::Two);
    drawCell(column, row, dest);
}

void CardAtlas::drawBack(Rectangle dest) const {
    drawCell(0, kBackRow, dest);
}

}
