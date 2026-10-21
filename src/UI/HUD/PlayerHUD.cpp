
#include "PlayerHUD.h"

#include <algorithm>
#include <sstream>
#include <string>
#include <iomanip>

#include "../../Graphics/UIPanel.h"
#include "../../Game/GameState.h"
#include "../../Fonts/UiFonts.h"

PlayerHUD::PlayerHUD(
    UIPanel& uiPanel,
    GameState& gameState,
    UiFonts& uiFonts
)
    : uiPanel(uiPanel),
    gameState(gameState),
    uiFonts(uiFonts)
{
}

void PlayerHUD::draw(
    const std::vector<uint32_t>& ownedIds,
    const std::unordered_map<uint32_t, RenderState>& renderStates,
    float screenW,
    float screenH,
    double fps,
    int pingMs,
    std::int64_t serverSessionSeconds,
    const std::string& serverInfoText
)
{
    // --------------------------------------------------------
    // Player statistics
    // --------------------------------------------------------

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

        const std::string hudText = hudStream.str();

        constexpr float hudFontScale = 1.0f;
        constexpr float paddingX = 14.0f;
        constexpr float panelHeight = 40.0f;

        const float textWidthPx =
            uiFonts.playerStats.measureWidth(hudText) *
            hudFontScale;

        const float panelWidth =
            textWidthPx + paddingX * 2.0f;

        constexpr float marginLeft = 10.0f;
        constexpr float marginBottom = 10.0f;

        const float panelCenterX =
            marginLeft + panelWidth * 0.5f;

        const float panelCenterY =
            screenH -
            marginBottom -
            panelHeight * 0.5f;

        uiPanel.draw(
            panelCenterX,
            panelCenterY,
            panelWidth,
            panelHeight,
            10.0f,

            // Background
            0.0f, 0.0f, 0.0f, 0.30f,

            // Border
            1.0f, 1.0f, 1.0f, 0.15f,

            1.0f,
            screenW,
            screenH
        );

        const float hudTextTopY =
            panelCenterY -
            uiFonts.playerStats.lineHeight() *
            hudFontScale * 0.5f;

        uiFonts.renderer.add(
            uiFonts.playerStats,
            hudText,
            marginLeft + paddingX,
            hudTextTopY,
            1.0f,
            1.0f,
            1.0f,
            1.0f,
            hudFontScale
        );

        uiFonts.renderer.flush(
            uiFonts.playerStats,
            screenW,
            screenH
        );
    }

    // --------------------------------------------------------
    // FPS + Ping
    // --------------------------------------------------------

    std::ostringstream performanceStream;

    // Сначала время подключения.
    performanceStream << "time:";

    if (serverSessionSeconds >= 0)
    {
        const std::int64_t hours =
            serverSessionSeconds / 3600;

        const std::int64_t minutes =
            (serverSessionSeconds % 3600) / 60;

        const std::int64_t seconds =
            serverSessionSeconds % 60;

        performanceStream
            << std::setfill('0')
            << std::setw(2) << hours << ':'
            << std::setw(2) << minutes << ':'
            << std::setw(2) << seconds;
    }
    else
    {
        performanceStream << "--:--:--";
    }

    // Затем FPS и Ping.
    performanceStream
        << " FPS:"
        << static_cast<int>(fps + 0.5)
        << " Ping:";

    if (pingMs >= 0)
        performanceStream << pingMs << " ms";
    else
        performanceStream << "--";

    const std::string performanceText =
        performanceStream.str();

    constexpr float performanceFontScale = 1.0f;
    constexpr float performanceMarginRight = 10.0f;

    const float performanceTextWidth =
        uiFonts.performance.measureWidth(performanceText) *
        performanceFontScale;

    const float performanceTextX =
        screenW -
        performanceMarginRight -
        performanceTextWidth;

    constexpr float performanceTextTopY = 2.0f;

    uiFonts.renderer.add(
        uiFonts.performance,
        performanceText,
        performanceTextX,
        performanceTextTopY,
        62.0f / 255.0f,
        117.0f / 255.0f,
        178.0f / 255.0f,
        1.0f,
        performanceFontScale
    );

    uiFonts.renderer.flush(
        uiFonts.performance,
        screenW,
        screenH
    );

    // Сохраняем прежний вертикальный ориентир realtimeinfo.
    constexpr float performancePanelHeight = 34.0f;

    const float performancePanelCenterY =
        screenH -
        10.0f -
        22.0f -
        10.0f +
        performancePanelHeight * 0.5f -
        16.0f;

    // --------------------------------------------------------
    // Server realtime info
    // --------------------------------------------------------

    if (!serverInfoText.empty())
    {
        constexpr float marginRight = 2.0f;
        constexpr float serverInfoFontScale = 1.0f;

        const float availableWidth =
            std::max(0.0f, screenW - marginRight * 2.0f);

        const float naturalTextWidth =
            uiFonts.serverInfo.measureWidth(serverInfoText);

        const float measuredWidth =
            naturalTextWidth * serverInfoFontScale;

        const float actualScale =
            measuredWidth > availableWidth && measuredWidth > 0.0f
            ? serverInfoFontScale * availableWidth / measuredWidth
            : serverInfoFontScale;

        const float serverInfoTextWidth =
            naturalTextWidth * actualScale;

        const float serverInfoX =
            screenW - marginRight - serverInfoTextWidth;

        const float serverInfoTopY =
            performancePanelCenterY +
            performancePanelHeight * 0.5f +
            11.0f;

        uiFonts.renderer.add(
            uiFonts.serverInfo,
            serverInfoText,
            serverInfoX,
            serverInfoTopY,
            62.0f / 255.0f,
            117.0f / 255.0f,
            178.0f / 255.0f,
            1.0f,
            actualScale
        );

        uiFonts.renderer.flush(
            uiFonts.serverInfo,
            screenW,
            screenH
        );
    }
}
