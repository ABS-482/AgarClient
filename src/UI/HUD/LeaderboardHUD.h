#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include <unordered_set>
#include "../../Game/Leaderboard.h"
#include "../../Graphics/Shader.h"

class UIPanel;
class UiTextRenderer;
class UiFont;
class World;
class SkinMesh;

class LeaderboardHUD
{
public:
    LeaderboardHUD(
        UIPanel& uiPanel,
        SkinMesh& skinMesh,
        UiTextRenderer& textRenderer,
        UiFont& font,
        World& world
    );

    ~LeaderboardHUD();

    // ownedIds передаётся снаружи, чтобы не копировать его заново.
    void draw(
        const std::vector<uint32_t>& ownedIds,
        float screenW,
        float screenH,
        uint8_t gameMode
    );

private:
    struct BadgeText
    {
        std::string text;
        float width = 0.0f;   // ширина при масштабе 1.0
    };

    const BadgeText& badgeText(uint16_t level);

    UIPanel& uiPanel;
    UiTextRenderer& textRenderer;
    UiFont& font;
    World& world;

    // Геометрия прямоугольника для фоновой текстуры.
    SkinMesh& skinMesh;

    // Шейдер и текстура фона Leaderboard.
    Shader leaderboardTextureShader;
    GLuint leaderboardTexture = 0;
    int leaderboardTextureWidth = 0;
    int leaderboardTextureHeight = 0;

    // Uniform-переменные шейдера.
    GLint leaderboardTextureCenter = -1;
    GLint leaderboardTextureHalfSize = -1;
    GLint leaderboardTextureScreenSize = -1;
    GLint leaderboardTextureSampler = -1;
    GLint leaderboardTextureTint = -1;

    std::unordered_map<uint16_t, BadgeText> badgeCache;

    static constexpr int kMaxRows = 20;
    std::array<std::string, kMaxRows> rankedText;

    std::shared_ptr<const std::vector<LeaderboardEntry>>
        ownershipSnapshot;

    std::unordered_set<uint32_t> highlightedLeaderboardIds;
    bool hasCachedGameMode = false;
    uint8_t cachedGameMode = 0;
};