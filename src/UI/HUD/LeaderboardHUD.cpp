#include "LeaderboardHUD.h"

#include <algorithm>
#include <string>
#include <cstdint>
#include <vector>
#include <utility>
#include <unordered_set>

#include "../../Graphics/UIPanel.h"
#include "../../Fonts/UiTextRenderer.h"
#include "../../Fonts/UiFont.h"
#include "../../Game/World.h"
#include "../../Graphics/Shader.h"
#include "../../Graphics/SkinMesh.h"
#include "../../Graphics/Shaders/UiTextureShader.h"
#include "../../Core/AssetPath.h"

#include <stb_image.h>
#include <iostream>

namespace
{
    GLuint loadLeaderboardTexture(
        const std::string& path,
        int& outWidth,
        int& outHeight
    )
    {
        int width = 0;
        int height = 0;
        int channels = 0;

        unsigned char* pixels = stbi_load(
            path.c_str(),
            &width,
            &height,
            &channels,
            4
        );

        if (pixels == nullptr)
        {
            std::cerr
                << "[LeaderboardHUD] Failed to load texture: "
                << path
                << " | "
                << stbi_failure_reason()
                << '\n';

            return 0;
        }

        outWidth = width;
        outHeight = height;

        GLuint texture = 0;

        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);

        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        glTexParameteri(
            GL_TEXTURE_2D,
            GL_TEXTURE_MIN_FILTER,
            GL_LINEAR
        );

        glTexParameteri(
            GL_TEXTURE_2D,
            GL_TEXTURE_MAG_FILTER,
            GL_LINEAR
        );

        glTexParameteri(
            GL_TEXTURE_2D,
            GL_TEXTURE_WRAP_S,
            GL_CLAMP_TO_EDGE
        );

        glTexParameteri(
            GL_TEXTURE_2D,
            GL_TEXTURE_WRAP_T,
            GL_CLAMP_TO_EDGE
        );

        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_RGBA8,
            width,
            height,
            0,
            GL_RGBA,
            GL_UNSIGNED_BYTE,
            pixels
        );

        glBindTexture(GL_TEXTURE_2D, 0);
        stbi_image_free(pixels);

        return texture;
    }

    bool isAlternativeLeaderboardMode(uint8_t gameMode)
    {
        switch (gameMode)
        {
        case 2:  // ARENA
        case 10: // EATFORSPEED_RACE
        case 11: // EATFORSPEED_ARCADE
        case 13: // FATBOY
        case 18: // SPACEINVADERS
        case 19: // TEAM
        case 21: // ZOMBIE
        case 24: // KRAKEN
        case 27: // BATTLEROYALE
        case 28: // TEAMBASEDEFEND
            return true;

        default:
            return false;
        }
    }
}

LeaderboardHUD::LeaderboardHUD(
    UIPanel& uiPanel,
    SkinMesh& skinMesh,
    UiTextRenderer& textRenderer,
    UiFont& font,
    World& world
)
    : uiPanel(uiPanel),
    skinMesh(skinMesh),
    textRenderer(textRenderer),
    font(font),
    world(world),
    leaderboardTextureShader(
        UiTextureShader::vertex,
        UiTextureShader::fragment
    )
{
    leaderboardTextureCenter =
        leaderboardTextureShader.uniformLocation("uCenter");

    leaderboardTextureHalfSize =
        leaderboardTextureShader.uniformLocation("uHalfSize");

    leaderboardTextureScreenSize =
        leaderboardTextureShader.uniformLocation("uScreenSize");

    leaderboardTextureSampler =
        leaderboardTextureShader.uniformLocation("uTexture");

    leaderboardTextureTint =
        leaderboardTextureShader.uniformLocation("uTint");

    leaderboardTexture = loadLeaderboardTexture(
        AssetPath::get("ui/leaderboard.png"),
        leaderboardTextureWidth,
        leaderboardTextureHeight
    );
}

const LeaderboardHUD::BadgeText& LeaderboardHUD::badgeText(uint16_t level)
{
    auto it = badgeCache.find(level);
    if (it != badgeCache.end())
        return it->second;

    BadgeText b;
    b.text = (level == 999)
        ? std::string("\xE2\x88\x9E")
        : std::to_string(level);
    b.width = font.measureWidth(b.text);

    return badgeCache.emplace(level, std::move(b)).first->second;
}

void LeaderboardHUD::draw(
    const std::vector<uint32_t>& ownedIds,
    float screenW,
    float screenH,
    uint8_t gameMode
)
{
    const auto leaderboardSnapshot = world.getLeaderboardSnapshot();

    if (!leaderboardSnapshot)
        return;

    const auto& leaderboard = *leaderboardSnapshot;

    const bool alternativeLeaderboard =
        isAlternativeLeaderboardMode(gameMode);

    if (ownershipSnapshot != leaderboardSnapshot ||
        !hasCachedGameMode ||
        cachedGameMode != gameMode)
    {
        ownershipSnapshot = leaderboardSnapshot;
        cachedGameMode = gameMode;
        hasCachedGameMode = true;

        for (int i = 0; i < kMaxRows; ++i)
        {
            if (static_cast<std::size_t>(i) < leaderboard.size())
            {
                rankedText[i] = alternativeLeaderboard
                    ? leaderboard[i].name
                    : std::to_string(i + 1) + ". " + leaderboard[i].name;
            }
            else
            {
                rankedText[i].clear();
            }
        }

        std::unordered_set<uint32_t> currentLeaderboardIds;
        currentLeaderboardIds.reserve(leaderboard.size());

        for (const auto& entry : leaderboard)
            currentLeaderboardIds.insert(entry.id);

        for (auto it = highlightedLeaderboardIds.begin();
            it != highlightedLeaderboardIds.end();)
        {
            if (currentLeaderboardIds.find(*it) ==
                currentLeaderboardIds.end())
            {
                it = highlightedLeaderboardIds.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }

    if (leaderboard.empty())
        return;

    constexpr int maxVisible = kMaxRows;

    constexpr float rowHeight = 17.0f;
    constexpr float paddingX = 5.0f;

    const int visibleCount =
        std::min(
            static_cast<int>(leaderboard.size()),
            maxVisible
        );

    const float panelWidth =
        leaderboardTextureWidth > 0
        ? static_cast<float>(leaderboardTextureWidth)
        : 250.0f;

    const float panelHeight =
        rowHeight * visibleCount;

    // --------------------------------------------------------
    // Позиция
    // --------------------------------------------------------

    constexpr float marginRight = 10.0f;
    constexpr float marginTop = 10.0f;

    const float panelCenterX =
        screenW - marginRight - panelWidth * 0.5f;

    const float panelCenterY =
        15.0f + panelHeight * 0.5f;

    // --------------------------------------------------------
    // Фон рейтинга — текстура оригинального Java-клиента
    // --------------------------------------------------------

    if (leaderboardTexture != 0)
    {
        leaderboardTextureShader.use();

        leaderboardTextureShader.setVec2(
            leaderboardTextureCenter,
            panelCenterX,
            panelCenterY
        );

        leaderboardTextureShader.setVec2(
            leaderboardTextureHalfSize,
            panelWidth * 0.5f,
            panelHeight * 0.5f
        );

        leaderboardTextureShader.setVec2(
            leaderboardTextureScreenSize,
            screenW,
            screenH
        );

        leaderboardTextureShader.setInt(
            leaderboardTextureSampler,
            0
        );

        leaderboardTextureShader.setVec4(
            leaderboardTextureTint,
            1.0f,
            1.0f,
            1.0f,
            1.0f
        );

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, leaderboardTexture);

        skinMesh.draw();

        glBindTexture(GL_TEXTURE_2D, 0);

    }
    else
    {
        // Резервный вариант, если PNG не загрузился.
        uiPanel.draw(
            panelCenterX,
            panelCenterY,
            panelWidth,
            panelHeight,
            12.0f,

            0.0f, 0.0f, 0.0f, 0.30f,

            1.0f, 1.0f, 1.0f, 0.35f,

            1.5f,

            screenW,
            screenH
        );
    }

    // --------------------------------------------------------
    // Текст
    // --------------------------------------------------------

    const float topY =
        panelCenterY - panelHeight * 0.5f;

    const float panelLeft =
        panelCenterX - panelWidth * 0.5f;

    const float rowX =
        panelLeft + paddingX;

    const float nameFontScale = 17.0f / 20.0f;

    // --------------------------------------------------------
    // Бейдж уровня — аналог Java TextButton из setLevel()/
    // setLevelSeason(). Circle для одной цифры, "пилюля" для
    // нескольких — ширина считается по реальному измеренному
    // тексту, а не по жёсткой таблице длин, как в Java.
    // --------------------------------------------------------

    constexpr float badgeHeight = 13.0f;
    constexpr float badgeFontScale = 12.0f / 20.0f;
    constexpr float badgePaddingX = 3.0f;
    constexpr float badgeMinWidth = 14.0f;
    constexpr float badgeGap = 1.0f;
    constexpr float rankGap = 1.0f;

    // Аналог JS .chatuserlevel
    // padding: 2px
    // border-radius: 15px
    // min-width: 12px
    // color: black
    auto drawLevelBadge =
        [&](
            uint16_t level,
            float cursorX,
            float rowY,
            float badgeR,
            float badgeG,
            float badgeB
            ) -> float
        {
            if (level == 0)
                return cursorX;

            const BadgeText& badge = badgeText(level);

            const float textWidth =
                badge.width * badgeFontScale;

            const float paddedTextWidth =
                textWidth + badgePaddingX * 2.0f;

            const float badgeWidth =
                paddedTextWidth > badgeMinWidth
                ? paddedTextWidth
                : badgeMinWidth;

            const float badgeCenterX =
                cursorX +
                badgeWidth * 0.5f;

            uiPanel.draw(
                badgeCenterX,
                rowY,
                badgeWidth,
                badgeHeight,
                badgeHeight * 0.5f,

                badgeR,
                badgeG,
                badgeB,
                1.0f,

                0.0f,
                0.0f,
                0.0f,
                0.0f,
                0.0f,

                screenW,
                screenH
            );

            const float badgeTextTop =
                rowY -
                font.lineHeight() * badgeFontScale * 0.5f;

            textRenderer.add(
                font,
                badge.text,
                badgeCenterX - textWidth * 0.5f,
                badgeTextTop,
                0.0f,
                0.0f,
                0.0f,
                1.0f,
                badgeFontScale
            );

            return cursorX + badgeWidth + badgeGap;
        };

    for (int i = 0; i < visibleCount; ++i)
    {
        const auto& entry =
            leaderboard[i];

        const bool currentlyOwned =
            std::find(
                ownedIds.begin(),
                ownedIds.end(),
                entry.id
            ) != ownedIds.end();

        if (currentlyOwned)
        {
            highlightedLeaderboardIds.insert(entry.id);
        }

        const bool isMe =
            highlightedLeaderboardIds.find(entry.id) !=
            highlightedLeaderboardIds.end();

        const float rowY =
            topY +
            rowHeight * i +
            rowHeight * 0.5f;

        float badgeCursorX = panelLeft;
        float cursorX = rowX;

        // Level
        badgeCursorX =
            drawLevelBadge(
                entry.userLevel,
                badgeCursorX,
                rowY,
                0.855f,
                0.647f,
                0.125f
            );

        // MonthLevel
        if (entry.hasSeason)
        {
            badgeCursorX =
                drawLevelBadge(
                    entry.userLevelSeason,
                    badgeCursorX,
                    rowY,
                    0.0f,
                    0.5f,
                    0.0f
                );
        }

        // Ник начинается после бейджей, если они есть.
        // Если бейджей нет — сохраняется обычный отступ paddingX.
        if (badgeCursorX > panelLeft)
        {
            cursorX = badgeCursorX;
        }

        // Дополнительный отступ перед местом.
        cursorX += rankGap;

        const float textR = 1.0f;
        const float textG = 1.0f;
        const float textB = isMe ? 0.0f : 1.0f;

        const float rankedTextTop =
            rowY -
            font.lineHeight() * nameFontScale * 0.5f;

        textRenderer.add(
            font,
            rankedText[i],
            cursorX,
            rankedTextTop,
            textR,
            textG,
            textB,
            1.0f,
            nameFontScale
        );
    }
}

LeaderboardHUD::~LeaderboardHUD()
{
    if (leaderboardTexture != 0)
    {
        glDeleteTextures(1, &leaderboardTexture);
        leaderboardTexture = 0;
    }
}