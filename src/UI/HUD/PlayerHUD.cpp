#include "PlayerHUD.h"

#include <sstream>
#include <string>

#include "../../Graphics/UIPanel.h"
#include "../../Graphics/TextRenderer.h"
#include "../../Graphics/Font.h"
#include "../../Game/GameState.h"

PlayerHUD::PlayerHUD(
    UIPanel& uiPanel,
    TextRenderer& textRenderer,
    Font& font,
    GameState& gameState
)
    : uiPanel(uiPanel),
    textRenderer(textRenderer),
    font(font),
    gameState(gameState)
{
}

void PlayerHUD::draw(
    const std::vector<uint32_t>& ownedIds,
    const std::unordered_map<uint32_t, RenderState>& renderStates,
    float screenW,
    float screenH,
    double fps
)
{
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

        constexpr float paddingX = 14.0f;
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

    // --------------------------------------------------------
    // FPS + Ping
    // --------------------------------------------------------

    std::ostringstream performanceStream;

    performanceStream
        << "FPS: "
        << static_cast<int>(fps + 0.5);

    const std::string performanceText =
        performanceStream.str();

    constexpr float performanceFontScale =
        20.0f / 70.0f;

    const float performanceTextWidth =
        font.measureWidth(performanceText) *
        performanceFontScale;

    const float performancePanelWidth =
        performanceTextWidth + 10.0f;

    const float performancePanelHeight =
        34.0f;

    const float performancePanelCenterX =
        screenW -
        10.0f -
        performancePanelWidth * 0.5f;

    const float performancePanelCenterY =
        screenH -
        10.0f -
        22.0f -
        10.0f +
        performancePanelHeight * 0.5f;

    uiPanel.draw(
        performancePanelCenterX,
        performancePanelCenterY,
        performancePanelWidth,
        performancePanelHeight,
        0.0f,

        0.1647f,
        0.3922f,
        0.5882f,
        0.5f,

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
        performanceText,
        performancePanelCenterX -
        performancePanelWidth * 0.5f +
        5.0f,
        performancePanelCenterY,
        performanceFontScale
    );
}