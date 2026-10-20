#include "LeaderboardHUD.h"

#include <algorithm>
#include <string>
#include <cstdint>
#include <vector>
#include <utility>
#include <unordered_set>

#include "../../Graphics/UIPanel.h"
#include "../../Graphics/TextRenderer.h"
#include "../../Graphics/Font.h"
#include "../../Game/World.h"

LeaderboardHUD::LeaderboardHUD(
    UIPanel& uiPanel,
    TextRenderer& textRenderer,
    Font& font,
    World& world
)
    : uiPanel(uiPanel),
    textRenderer(textRenderer),
    font(font),
    world(world)
{
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
    float screenH
)
{
    const auto leaderboardSnapshot = world.getLeaderboardSnapshot();

    if (!leaderboardSnapshot)
        return;

    const auto& leaderboard = *leaderboardSnapshot;

    if (ownershipSnapshot != leaderboardSnapshot)
    {
        ownershipSnapshot = leaderboardSnapshot;

        for (int i = 0; i < kMaxRows; ++i)
        {
            if (static_cast<std::size_t>(i) < leaderboard.size())
            {
                rankedText[i] =
                    std::to_string(i + 1) + ". " + leaderboard[i].name;
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

    constexpr float rowHeight = 26.0f;
    constexpr float paddingX = 10.0f;
    constexpr float paddingY = 12.0f;

    const int visibleCount =
        std::min(
            static_cast<int>(leaderboard.size()),
            maxVisible
        );

    const float panelWidth = 250.0f;

    const float panelHeight =
        paddingY * 2.0f +
        rowHeight * visibleCount;

    // --------------------------------------------------------
    // Позиция
    // --------------------------------------------------------

    constexpr float marginRight = 10.0f;
    constexpr float marginTop = 10.0f;

    const float panelCenterX =
        screenW -
        marginRight -
        panelWidth * 0.5f;

    const float panelCenterY =
        marginTop +
        panelHeight * 0.5f;

    // --------------------------------------------------------
    // Панель
    // --------------------------------------------------------

    uiPanel.draw(
        panelCenterX,
        panelCenterY,
        panelWidth,
        panelHeight,
        12.0f,

        // background
        0.0f,
        0.0f,
        0.0f,
        0.30f,

        // border
        1.0f,
        1.0f,
        1.0f,
        0.35f,

        1.5f,

        screenW,
        screenH
    );

    // --------------------------------------------------------
    // Текст
    // --------------------------------------------------------

    const float topY =
        panelCenterY -
        panelHeight * 0.5f +
        paddingY;

    const float rowX =
        panelCenterX -
        panelWidth * 0.5f +
        paddingX;

    constexpr float nameFontScale = 0.25f;

    // --------------------------------------------------------
    // Бейдж уровня — аналог Java TextButton из setLevel()/
    // setLevelSeason(). Circle для одной цифры, "пилюля" для
    // нескольких — ширина считается по реальному измеренному
    // тексту, а не по жёсткой таблице длин, как в Java.
    // --------------------------------------------------------

    constexpr float badgeHeight = 20.0f;
    constexpr float badgeFontScale = 13.0f / 70.0f;
    constexpr float badgePaddingX = 6.0f;
    constexpr float badgeMinWidth = 18.0f;
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

            textRenderer.addTextColored(
                font,
                badge.text,
                badgeCenterX,
                rowY,
                badgeFontScale,
                0.0f,
                0.0f,
                0.0f
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

        float cursorX = rowX;

        // Level
        cursorX =
            drawLevelBadge(
                entry.userLevel,
                cursorX,
                rowY,
                0.855f,
                0.647f,
                0.125f
            );

        // MonthLevel
        if (entry.hasSeason)
        {
            cursorX =
                drawLevelBadge(
                    entry.userLevelSeason,
                    cursorX,
                    rowY,
                    0.0f,
                    0.5f,
                    0.0f
                );
        }

        // Дополнительный отступ перед местом.
        cursorX += rankGap;

        const float textR = 1.0f;
        const float textG = 1.0f;
        const float textB = isMe ? 0.0f : 1.0f;

        textRenderer.addTextLeftAlignedColored(
            font,
            rankedText[i],
            cursorX,
            rowY,
            nameFontScale,
            textR,
            textG,
            textB
        );
    }
}