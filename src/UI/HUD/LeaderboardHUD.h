#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include <unordered_set>
#include "../../Game/Leaderboard.h"

class UIPanel;
class TextRenderer;
class Font;
class World;

class LeaderboardHUD
{
public:
    LeaderboardHUD(
        UIPanel& uiPanel,
        TextRenderer& textRenderer,
        Font& font,
        World& world
    );

    // ownedIds передаётся снаружи, чтобы не копировать его заново.
    void draw(
        const std::vector<uint32_t>& ownedIds,
        float screenW,
        float screenH
    );

private:
    struct BadgeText
    {
        std::string text;
        float width = 0.0f;   // ширина при масштабе 1.0
    };

    const BadgeText& badgeText(uint16_t level);

    UIPanel& uiPanel;
    TextRenderer& textRenderer;
    Font& font;
    World& world;

    std::unordered_map<uint16_t, BadgeText> badgeCache;

    static constexpr int kMaxRows = 10;
    std::array<std::string, kMaxRows> rankedText;

    std::shared_ptr<const std::vector<LeaderboardEntry>>
        ownershipSnapshot;

    std::unordered_set<uint32_t> highlightedLeaderboardIds;
};