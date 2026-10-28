
#include "PlayerHUD.h"

#include <algorithm>
#include <sstream>
#include <string>
#include <iomanip>

#include "../../Graphics/UIPanel.h"
#include "../../Graphics/Camera.h"
#include "../../Game/GameState.h"
#include "../../Fonts/UiFonts.h"

PlayerHUD::PlayerHUD(
    UIPanel& uiPanel,
    GameState& gameState,
    UiFonts& uiFonts,
    Camera& camera
)
    : uiPanel(uiPanel),
    gameState(gameState),
    uiFonts(uiFonts),
    camera(camera)
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
    const std::string& serverInfoText,
    uint8_t gameMode
)
{


    // --------------------------------------------------------
// Player statistics — Java-style HUD
// --------------------------------------------------------

    constexpr float fontScale = 1.0f;

    // Цвет FPS, Ping и serverInfo.
    constexpr float r = 62.0f / 255.0f;
    constexpr float g = 117.0f / 255.0f;
    constexpr float b = 178.0f / 255.0f;
    constexpr float a = 1.0f;

    // Нижняя часть экрана.
    const float y = screenH - 16.0f;

    // Горизонтальное расположение.
    constexpr float xOffset = 13.0f;

    // По умолчанию показываем текущую позицию камеры.
    // Это используется в режиме наблюдателя.
    int displayedX = static_cast<int>(camera.pos.x);
    int displayedY = static_cast<int>(camera.pos.y);

    // Если есть свои клетки, используем координаты
    // первой клетки с учётом интерполяции.
    if (!ownedIds.empty())
    {
        const auto firstRsIt =
            renderStates.find(ownedIds[0]);

        if (firstRsIt != renderStates.end())
        {
            displayedX =
                static_cast<int>(firstRsIt->second.x);

            displayedY =
                static_cast<int>(firstRsIt->second.y);
        }
    }

    // Координаты отображаются всегда.
    uiFonts.renderer.add(
        uiFonts.playerStats,
        "x:" + std::to_string(displayedX),
        xOffset, y,
        r, g, b, a,
        fontScale
    );

    uiFonts.renderer.add(
        uiFonts.playerStats,
        "y:" + std::to_string(displayedY),
        xOffset + 66.0f, y,
        r, g, b, a,
        fontScale
    );

    // Масса и рекорд — только при наличии своих клеток.
    if (!ownedIds.empty())
    {
        std::ostringstream massStream;

        massStream
            << "mass:"
            << static_cast<int>(gameState.currentScore)
            << "|" << ownedIds.size();

        uiFonts.renderer.add(
            uiFonts.playerStats,
            massStream.str(),
            xOffset + 159.0f, y,
            r, g, b, a,
            fontScale
        );

        uiFonts.renderer.add(
            uiFonts.playerStats,
            "max:" +
            std::to_string(
                static_cast<int>(gameState.bestScoreSoFar)
            ),
            xOffset + 280.0f, y,
            r, g, b, a,
            fontScale
        );
    }

    // Выводим все добавленные строки, включая координаты
    // в режиме наблюдателя.
    uiFonts.renderer.flush(
        uiFonts.playerStats,
        screenW,
        screenH
    );

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
