#include "LeaderboardHUD.h"

#include <algorithm>
#include <string>

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

void LeaderboardHUD::draw(
    float screenW,
    float screenH
)
{
    auto leaderboard = world.getLeaderboard();

    if (leaderboard.empty())
        return;

    constexpr int maxVisible = 10;

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

    auto formatLevel =
        [](uint16_t level) -> std::string
        {
            if (level == 999)
                return "\xE2\x88\x9E";

            return std::to_string(level);
        };

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

            const std::string text =
                formatLevel(level);

            const float textWidth =
                font.measureWidth(text) *
                badgeFontScale;

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
                text,
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

        const std::string rankPrefix =
            std::to_string(i + 1) +
            ". ";

        textRenderer.addTextLeftAligned(
            font,
            rankPrefix,
            cursorX,
            rowY,
            nameFontScale
        );

        cursorX +=
            font.measureWidth(rankPrefix) *
            nameFontScale;

        textRenderer.addTextLeftAligned(
            font,
            entry.name,
            cursorX,
            rowY,
            nameFontScale
        );
    }
}