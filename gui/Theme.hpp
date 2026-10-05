#pragma once

#include <raylib.h>

namespace gui::theme {

inline constexpr int kScreenWidthPx = 1280;
inline constexpr int kScreenHeightPx = 720;
inline constexpr float kScreenWidth = static_cast<float>(kScreenWidthPx);
inline constexpr float kScreenHeight = static_cast<float>(kScreenHeightPx);

inline constexpr Color kFeltCenter{22, 118, 76, 255};
inline constexpr Color kFeltEdge{6, 46, 30, 255};
inline constexpr Color kGold{218, 180, 96, 255};
inline constexpr Color kText{240, 236, 222, 255};
inline constexpr Color kInkDark{30, 26, 18, 255};
inline constexpr Color kPill{8, 40, 26, 210};
inline constexpr Color kBanner{10, 22, 16, 255};
inline constexpr Color kShoe{40, 26, 18, 255};
inline constexpr Color kWin{120, 220, 140, 255};
inline constexpr Color kLoss{226, 86, 86, 255};

inline constexpr Color kButton{24, 70, 48, 255};
inline constexpr Color kButtonHover{34, 98, 66, 255};
inline constexpr Color kButtonDisabled{20, 44, 32, 160};

}
