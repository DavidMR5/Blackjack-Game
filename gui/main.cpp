
#include "CardAtlas.hpp"
#include "TableView.hpp"
#include "Theme.hpp"

#include "agents/BasicStrategyAgent.hpp"
#include "blackjack/BlackjackGame.hpp"

#include <raylib.h>
#include <rlgl.h>

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <random>
#include <string>
#include <string_view>
#include <utility>

using namespace blackjack;

namespace gui {

namespace {

struct Options {
    std::uint32_t seed = std::random_device{}();
    bool dealerPeeks = false;
    std::string screenshotPrefix;
};

Font loadUiFont() {
    const char* candidates[] = {
        "C:/Windows/Fonts/segoeuib.ttf",
        "C:/Windows/Fonts/arialbd.ttf",
        "/System/Library/Fonts/Supplemental/Arial Bold.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf",
        "/usr/share/fonts/TTF/DejaVuSans-Bold.ttf",
    };

    for (const char* path : candidates) {
        if (FileExists(path)) {
            Font font = LoadFontEx(path, 96, nullptr, 0);
            if (IsFontValid(font)) {
                GenTextureMipmaps(&font.texture);
                SetTextureFilter(font.texture, TEXTURE_FILTER_TRILINEAR);
                return font;
            }
        }
    }

    return GetFontDefault();
}

struct Button {
    const char* label;
    const char* key;
    Rectangle rect;
};

class App {
public:
    App(const CardAtlas& atlas, const Font& font, Options options)
        : font_(font),
          options_(std::move(options)),
          rules_(makeRules(options_.dealerPeeks)),
          game_(rules_, options_.seed),
          view_(atlas, font) {
        if (!options_.screenshotPrefix.empty()) {
            autoplay_ = true;
            showHint_ = true;
        }
    }

    bool wantsToQuit() const { return quit_; }

    void captureIfRequested() {
        if (pendingShot_.empty()) {
            return;
        }

        // Flush pending draws before reading the framebuffer.
        rlDrawRenderBatchActive();
        TakeScreenshot(pendingShot_.c_str());
        pendingShot_.clear();

        if (shotsTaken_ == 2) {
            quit_ = true;
        }
    }

    void update(float dt) {
        view_.update(dt);

        if (auto result = view_.takeFinishedResult()) {
            record(*result);
        }

        handleInput();

        if (autoplay_) {
            updateAutoplay(dt);
        }

        if (!options_.screenshotPrefix.empty()) {
            updateScreenshots(dt);
        }
    }

    void draw() const {
        drawFelt();
        view_.draw();
        drawHud();
        drawHint();
        drawButtons();
    }

private:
    static Rules makeRules(bool dealerPeeks) {
        Rules rules;
        rules.dealerPeeks = dealerPeeks;
        return rules;
    }

    bool canDeal() const { return !game_.isAwaitingPlayer() && !view_.isAnimating(); }
    bool canAct() const { return game_.isAwaitingPlayer() && !view_.isAnimating(); }

    void deal() {
        game_.beginRound(&view_);
        aiTimer_ = 0.0f;
    }

    void act(Action action) {
        game_.act(action);
        aiTimer_ = 0.0f;
    }

    void record(const RoundResult& result) {
        balance_ += result.reward;
        ++hands_;
        aiTimer_ = 0.0f;

        if (result.reward > 0.0) {
            ++wins_;
        } else if (result.reward < 0.0) {
            ++losses_;
        } else {
            ++pushes_;
        }
    }

    void toggleRules() {
        rules_.dealerPeeks = !rules_.dealerPeeks;
        game_ = BlackjackGame(rules_, options_.seed + static_cast<std::uint32_t>(hands_));
    }

    static constexpr float kButtonWidth = 150.0f;
    static constexpr float kButtonHeight = 50.0f;
    static constexpr float kButtonGap = 18.0f;
    static constexpr float kButtonY = 652.0f;

    static Rectangle bottomButton(int index) {
        const float total = 4.0f * kButtonWidth + 3.0f * kButtonGap;
        const float x = theme::kScreenWidth * 0.5f - total * 0.5f +
                        static_cast<float>(index) * (kButtonWidth + kButtonGap);
        return {x, kButtonY, kButtonWidth, kButtonHeight};
    }

    static constexpr Rectangle kRulesToggle{24.0f, 112.0f, 250.0f, 32.0f};
    static constexpr Rectangle kAutoToggle{24.0f, 150.0f, 250.0f, 32.0f};

    bool clicked(Rectangle rect) const {
        return IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
               CheckCollisionPointRec(GetMousePosition(), rect);
    }

    void handleInput() {
        const bool keyDeal = IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER);

        if (!autoplay_) {
            if (canDeal() && (keyDeal || clicked(bottomButton(0)))) {
                deal();
            } else if (canAct() && (IsKeyPressed(KEY_H) || clicked(bottomButton(1)))) {
                act(Action::Hit);
            } else if (canAct() && (IsKeyPressed(KEY_S) || clicked(bottomButton(2)))) {
                act(Action::Stand);
            }
        }

        if (IsKeyPressed(KEY_B) || clicked(bottomButton(3))) {
            showHint_ = !showHint_;
        }

        if (canDeal() && (IsKeyPressed(KEY_R) || clicked(kRulesToggle))) {
            toggleRules();
        }

        if (IsKeyPressed(KEY_A) || clicked(kAutoToggle)) {
            autoplay_ = !autoplay_;
            aiTimer_ = 0.0f;
        }
    }

    void updateAutoplay(float dt) {
        aiTimer_ += dt;

        if (canAct() && aiTimer_ > 0.8f) {
            act(advisor_.chooseAction(game_.getState()));
        } else if (canDeal() && aiTimer_ > 1.6f) {
            deal();
        }
    }

    void updateScreenshots(float dt) {
        shotTimer_ += dt;

        if (shotsTaken_ == 0 && canAct() && aiTimer_ > 0.6f) {
            pendingShot_ = options_.screenshotPrefix + "_decision.png";
            ++shotsTaken_;
        } else if (shotsTaken_ == 1 && canDeal() && hands_ > 0 && aiTimer_ > 1.0f) {
            pendingShot_ = options_.screenshotPrefix + "_result.png";
            ++shotsTaken_;
        }

        if (shotTimer_ > 60.0f) {
            quit_ = true;
        }
    }

    void drawText(const char* text, Vector2 position, float size, Color color) const {
        DrawTextEx(font_, text, position, size, 1.0f, color);
    }

    void drawCentered(const char* text, Vector2 center, float size, Color color) const {
        const Vector2 dim = MeasureTextEx(font_, text, size, 1.0f);
        drawText(text, {center.x - dim.x * 0.5f, center.y - dim.y * 0.5f}, size, color);
    }

    void drawFelt() const {
        ClearBackground(theme::kFeltEdge);
        DrawCircleGradient(theme::kScreenWidthPx / 2, 330, 900.0f, theme::kFeltCenter, theme::kFeltEdge);

        const Color line = ColorAlpha(theme::kGold, 0.35f);
        DrawRing({theme::kScreenWidth * 0.5f, -470.0f}, 830.0f, 833.0f, 58.0f, 122.0f, 96, line);
        drawCentered("BLACKJACK PAYS 3 TO 2", {theme::kScreenWidth * 0.5f, 336.0f}, 26.0f,
                     ColorAlpha(theme::kGold, 0.55f));

        const char* rules = rules_.dealerPeeks
            ? "Dealer must stand on soft 17  -  American rules (dealer peeks)"
            : "Dealer must stand on soft 17  -  European rules (no hole card)";
        drawCentered(rules, {theme::kScreenWidth * 0.5f, 398.0f}, 17.0f,
                     ColorAlpha(theme::kText, 0.35f));
    }

    void drawToggle(Rectangle rect, const char* key, const std::string& label) const {
        const bool hover = CheckCollisionPointRec(GetMousePosition(), rect);
        DrawRectangleRounded(rect, 0.5f, 12, hover ? theme::kButtonHover : ColorAlpha(theme::kPill, 0.8f));
        drawText(key, {rect.x + 12.0f, rect.y + 7.0f}, 18.0f, theme::kGold);
        drawText(label.c_str(), {rect.x + 36.0f, rect.y + 7.0f}, 18.0f, theme::kText);
    }

    void drawHud() const {
        const Rectangle panel{16.0f, 16.0f, 266.0f, 174.0f};
        DrawRectangleRounded(panel, 0.12f, 12, ColorAlpha(theme::kPill, 0.75f));

        char line[96];
        std::snprintf(line, sizeof(line), "%+.1f", balance_);
        drawText("BALANCE", {32.0f, 28.0f}, 16.0f, ColorAlpha(theme::kText, 0.6f));
        drawText(line, {32.0f, 46.0f}, 34.0f, balance_ >= 0.0 ? theme::kWin : theme::kLoss);
        drawText("units", {48.0f + MeasureTextEx(font_, line, 34.0f, 1.0f).x, 58.0f}, 18.0f,
                 ColorAlpha(theme::kText, 0.6f));

        std::snprintf(line, sizeof(line), "Hands %d    W %d   L %d   P %d",
                      hands_, wins_, losses_, pushes_);
        drawText(line, {32.0f, 84.0f}, 17.0f, theme::kText);

        drawToggle(kRulesToggle, "R", rules_.dealerPeeks ? "Rules: American" : "Rules: European");
        drawToggle(kAutoToggle, "A", autoplay_ ? "AI autoplay: ON" : "AI autoplay: off");
    }

    void drawHint() const {
        if (!game_.isAwaitingPlayer() || view_.isAnimating() || !(showHint_ || autoplay_)) {
            return;
        }

        const Action advice = BasicStrategyAgent{}.chooseAction(game_.getState());
        const char* title = autoplay_ ? "AI (basic strategy)" : "Basic strategy says";
        const char* move = advice == Action::Hit ? "HIT" : "STAND";

        const Rectangle bubble{120.0f, 470.0f, 250.0f, 86.0f};
        DrawRectangleRounded(bubble, 0.3f, 12, ColorAlpha(theme::kBanner, 0.85f));
        DrawRectangleRoundedLinesEx(bubble, 0.3f, 12, 2.0f, ColorAlpha(theme::kGold, 0.7f));
        drawText(title, {bubble.x + 18.0f, bubble.y + 12.0f}, 18.0f, ColorAlpha(theme::kText, 0.75f));
        drawText(move, {bubble.x + 18.0f, bubble.y + 36.0f}, 38.0f, theme::kGold);
    }

    void drawButton(Rectangle rect, const char* label, const char* key, bool enabled, bool active) const {
        const bool hover = enabled && CheckCollisionPointRec(GetMousePosition(), rect);
        const Color fill = !enabled ? theme::kButtonDisabled
                         : (hover || active) ? theme::kButtonHover
                         : theme::kButton;

        DrawRectangleRounded({rect.x + 2.0f, rect.y + 5.0f, rect.width, rect.height}, 0.4f, 12,
                             ColorAlpha(BLACK, enabled ? 0.35f : 0.15f));
        DrawRectangleRounded(rect, 0.4f, 12, fill);
        DrawRectangleRoundedLinesEx(rect, 0.4f, 12, 2.0f,
                                    ColorAlpha(theme::kGold, enabled ? 0.85f : 0.25f));

        const float alpha = enabled ? 1.0f : 0.35f;
        drawCentered(label, {rect.x + rect.width * 0.5f, rect.y + rect.height * 0.5f - 6.0f}, 22.0f,
                     ColorAlpha(theme::kText, alpha));
        drawCentered(key, {rect.x + rect.width * 0.5f, rect.y + rect.height * 0.5f + 14.0f}, 13.0f,
                     ColorAlpha(theme::kGold, 0.8f * alpha));
    }

    void drawButtons() const {
        const bool manual = !autoplay_;
        drawButton(bottomButton(0), "DEAL", "SPACE", manual && canDeal(), false);
        drawButton(bottomButton(1), "HIT", "H", manual && canAct(), false);
        drawButton(bottomButton(2), "STAND", "S", manual && canAct(), false);
        drawButton(bottomButton(3), "HINT", "B", true, showHint_);

        if (hands_ == 0 && canDeal() && !autoplay_) {
            drawCentered("Press DEAL to start", {theme::kScreenWidth * 0.5f, 520.0f}, 28.0f,
                         ColorAlpha(theme::kText, 0.8f));
        }
    }

    const Font& font_;
    Options options_;
    Rules rules_;
    BlackjackGame game_;
    TableView view_;
    BasicStrategyAgent advisor_;

    double balance_ = 0.0;
    int hands_ = 0;
    int wins_ = 0;
    int losses_ = 0;
    int pushes_ = 0;

    bool showHint_ = false;
    bool autoplay_ = false;
    float aiTimer_ = 0.0f;

    float shotTimer_ = 0.0f;
    int shotsTaken_ = 0;
    std::string pendingShot_;
    bool quit_ = false;
};

Options parseArgs(int argc, char** argv) {
    Options options;

    for (int i = 1; i < argc; ++i) {
        const std::string_view arg = argv[i];

        if (arg == "--seed" && i + 1 < argc) {
            options.seed = static_cast<std::uint32_t>(std::strtoul(argv[++i], nullptr, 10));
        } else if (arg == "--peek") {
            options.dealerPeeks = true;
        } else if (arg == "--screenshot" && i + 1 < argc) {
            options.screenshotPrefix = argv[++i];
        } else {
            std::printf("Usage: blackjack_gui [--seed N] [--peek] [--screenshot PREFIX]\n");
            std::exit(arg == "--help" ? 0 : 1);
        }
    }

    return options;
}

}
}

int main(int argc, char** argv) {
    const gui::Options options = gui::parseArgs(argc, argv);

    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT);
    SetTraceLogLevel(LOG_WARNING);
    InitWindow(gui::theme::kScreenWidthPx, gui::theme::kScreenHeightPx, "Blackjack AI Lab");
    SetTargetFPS(60);
    SetExitKey(KEY_ESCAPE);

    const Font font = gui::loadUiFont();

    {
        const gui::CardAtlas atlas(font);
        gui::App app(atlas, font, options);

        while (!WindowShouldClose() && !app.wantsToQuit()) {
            app.update(GetFrameTime());

            BeginDrawing();
            app.draw();
            app.captureIfRequested();
            EndDrawing();
        }
    }

    if (font.texture.id != GetFontDefault().texture.id) {
        UnloadFont(font);
    }

    CloseWindow();
    return 0;
}
