#include "GameHUD.h"

#include <algorithm>
#include <sstream>
#include <string>

GameHUD::GameHUD(
    UIPanel& uiPanel,
    TextRenderer& textRenderer,
    Font& font,
    Camera& camera,
    World& world,
    GameState& gameState
)
    : uiPanel(uiPanel),
    textRenderer(textRenderer),
    font(font),
    camera(camera),
    world(world),
    gameState(gameState)
{
}

void GameHUD::draw(
    const std::vector<uint32_t>& ownedIds,
    const std::unordered_map<uint32_t, RenderState>& renderStates,
    float screenW,
    float screenH
)
{
    constexpr float rectR = 0.1647f;
    constexpr float rectG = 0.3922f;
    constexpr float rectB = 0.5882f;
    constexpr float rectA = 0.5f;

    textRenderer.begin();

    // -------------------------
    // Leaderboard
    // -------------------------

    auto leaderboard = world.getLeaderboard();

    if (!leaderboard.empty())
    {
        constexpr int maxVisible = 10;
        constexpr float rowHeight = 28.0f;
        constexpr float padding = 16.0f;

        const int visibleCount =
            std::min(
                static_cast<int>(leaderboard.size()),
                maxVisible
            );

        const float panelWidth = 240.0f;

        const float panelHeight =
            padding * 2.0f +
            rowHeight * visibleCount;

        const float panelCenterX =
            screenW -
            panelWidth * 0.5f -
            16.0f;

        const float panelCenterY =
            panelHeight * 0.5f +
            16.0f;

        uiPanel.draw(
            panelCenterX,
            panelCenterY,
            panelWidth,
            panelHeight,
            0.0f,
            rectR,
            rectG,
            rectB,
            rectA,
            0.0f,
            0.0f,
            0.0f,
            0.0f,
            0.0f,
            screenW,
            screenH
        );

        const float topY =
            panelCenterY -
            panelHeight * 0.5f +
            padding;

        const float rowX =
            screenW -
            panelWidth -
            16.0f +
            padding;

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
                0.28f
            );
        }
    }

    // -------------------------
    // Chat
    // -------------------------

    auto chatMessages =
        world.getChatMessages();

    if (!chatMessages.empty())
    {
        constexpr int maxVisible = 14;
        constexpr float rowHeight = 22.0f;
        constexpr float padding = 12.0f;

        const int visibleCount =
            std::min(
                static_cast<int>(chatMessages.size()),
                maxVisible
            );

        const float panelWidth = 300.0f;

        const float panelHeight =
            padding * 2.0f +
            rowHeight * maxVisible;

        const float panelCenterX =
            panelWidth * 0.5f +
            10.0f;

        const float hudReservedHeight =
            ownedIds.empty()
            ? 16.0f
            : 52.0f;

        const float panelCenterY =
            screenH -
            hudReservedHeight -
            panelHeight * 0.5f;

        uiPanel.draw(
            panelCenterX,
            panelCenterY,
            panelWidth,
            panelHeight,
            12.0f,
            0.08f,
            0.08f,
            0.12f,
            0.65f,
            1.0f,
            1.0f,
            1.0f,
            0.12f,
            1.5f,
            screenW,
            screenH
        );

        const float topY =
            panelCenterY +
            panelHeight * 0.5f -
            padding -
            rowHeight * visibleCount;

        const int startIdx =
            static_cast<int>(chatMessages.size()) -
            visibleCount;

        for (int i = 0; i < visibleCount; ++i)
        {
            const auto& msg =
                chatMessages[startIdx + i];

            const std::string line =
                msg.isPlayerEnter
                ? msg.name + " enters the game"
                : msg.name + ": " + msg.message;

            const float rowY =
                topY +
                rowHeight * i +
                rowHeight * 0.5f;

            const float rowX =
                10.0f +
                padding;

            textRenderer.addTextLeftAligned(
                font,
                line,
                rowX,
                rowY,
                0.24f
            );
        }
    }

    // -------------------------
    // Personal HUD
    // -------------------------

    if (!ownedIds.empty())
    {
        const auto firstRsIt =
            renderStates.find(ownedIds[0]);

        const float firstX =
            firstRsIt != renderStates.end()
            ? firstRsIt->second.x
            : 0.0f;

        const float firstY =
            firstRsIt != renderStates.end()
            ? firstRsIt->second.y
            : 0.0f;

        std::ostringstream hudStream;

        hudStream
            << "x:"
            << static_cast<int>(firstX)
            << " y:"
            << static_cast<int>(firstY)
            << ". Your best: "
            << static_cast<int>(gameState.bestScoreSoFar)
            << ". Now: "
            << static_cast<int>(gameState.currentScore)
            << " ("
            << ownedIds.size()
            << ")";

        const std::string hudText =
            hudStream.str();

        constexpr float hudFontScale =
            20.0f / 70.0f;

        const float textWidthPx =
            font.measureWidth(hudText) *
            hudFontScale;

        const float panelWidth =
            textWidthPx + 10.0f;

        const float panelHeight =
            34.0f;

        const float panelCenterX =
            10.0f +
            panelWidth * 0.5f;

        const float panelCenterY =
            screenH -
            10.0f -
            22.0f -
            10.0f +
            panelHeight * 0.5f;

        uiPanel.draw(
            panelCenterX,
            panelCenterY,
            panelWidth,
            panelHeight,
            0.0f,
            rectR,
            rectG,
            rectB,
            rectA,
            0.0f,
            0.0f,
            0.0f,
            0.0f,
            0.0f,
            screenW,
            screenH
        );

        textRenderer.addTextLeftAligned(
            font,
            hudText,
            15.0f,
            panelCenterY,
            hudFontScale
        );
    }

    textRenderer.end(
        font,
        screenW,
        screenH,
        1.0f,
        1.0f,
        1.0f
    );
}