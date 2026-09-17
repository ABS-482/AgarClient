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
        // Тёмная панель в стиле JS
        // background: rgba(0, 0, 0, .30)
        // border: 1px solid rgba(255, 255, 255, .12)
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

        // --------------------------------------------------------
        // Typography
        // --------------------------------------------------------

        constexpr float hudFontScale =
            20.0f / 70.0f;

        const float textWidthPx =
            font.measureWidth(hudText) *
            hudFontScale;

        // Небольшие внутренние отступы по горизонтали.
        constexpr float paddingX = 14.0f;

        // Высота как у компактного HUD из JS.
        constexpr float panelHeight = 40.0f;

        const float panelWidth =
            textWidthPx +
            paddingX * 2.0f;

        // --------------------------------------------------------
        // Position
        // --------------------------------------------------------

        constexpr float marginLeft = 10.0f;
        constexpr float marginBottom = 10.0f;

        const float panelCenterX =
            marginLeft +
            panelWidth * 0.5f;

        const float panelCenterY =
            screenH -
            marginBottom -
            panelHeight * 0.5f;

        // --------------------------------------------------------
        // Dark strict panel
        // background: rgba(0, 0, 0, .30)
        // border:     rgba(255, 255, 255, .15)
        // --------------------------------------------------------

        uiPanel.draw(
            panelCenterX,
            panelCenterY,
            panelWidth,
            panelHeight,
            20.0f,

            // background
            0.0f,
            0.0f,
            0.0f,
            0.30f,

            // border
            1.0f,
            1.0f,
            1.0f,
            0.15f,

            1.0f,

            screenW,
            screenH
        );

        textRenderer.addTextLeftAligned(
            font,
            hudText,
            marginLeft + paddingX,
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