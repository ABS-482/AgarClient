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
    constexpr float paddingX = 14.0f;
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
        0.12f,

        1.0f,

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

    for (int i = 0; i < visibleCount; ++i)
    {
        const auto& entry =
            leaderboard[i];

        const std::string line =
            std::to_string(i + 1) +
            ". " +
            entry.name;

        const float rowY =
            topY +
            rowHeight * i +
            rowHeight * 0.5f;

        textRenderer.addTextLeftAligned(
            font,
            line,
            rowX,
            rowY,
            0.24f
        );
    }
}