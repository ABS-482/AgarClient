
#include "GameHUD.h"

#include <algorithm>
#include <sstream>
#include <string>

#include "../FrameProfiler.h"

GameHUD::GameHUD(
    UIPanel& uiPanel,
    TextRenderer& textRenderer,
    Font& font,
    Camera& camera,
    World& world,
    GameState& gameState,
    Shader& chatSkinShader,
    SkinMesh& skinMesh,
    SkinManager& skinManager,
    UiFonts& uiFonts,
    UserSkinDatabase& userSkinDatabase
)
    : uiPanel(uiPanel),
    textRenderer(textRenderer),
    font(font),
    camera(camera),
    world(world),
    gameState(gameState),
    uiFonts(uiFonts),
    leaderboardHUD(
        uiPanel,
        skinMesh,
        uiFonts.renderer,
        uiFonts.leaderboard,
        world
    ),
    playerHUD(
        uiPanel,
        gameState,
        uiFonts,
        camera
    ),
    chatHUD(
        uiPanel,
        textRenderer,
        font,
        world,
        chatSkinShader,
        skinMesh,
        skinManager,
        uiFonts,
        userSkinDatabase
    )
{
}

void GameHUD::draw(
    FrameProfiler& profiler,
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
    textRenderer.begin();

    // --------------------------------------------------------
    // Leaderboard
    // --------------------------------------------------------

    {
        FrameProfiler::ScopedTimer timer(
            profiler,
            FrameProfiler::Leaderboard
        );

        uiFonts.renderer.begin();

        leaderboardHUD.draw(
            ownedIds,
            screenW,
            screenH,
            gameMode
        );

        // Таблица лидеров использует собственный атлас шрифта.
        uiFonts.renderer.flush(
            uiFonts.leaderboard,
            screenW,
            screenH
        );
    }

    // --------------------------------------------------------
    // Chat
    // --------------------------------------------------------

    {
        FrameProfiler::ScopedTimer timer(
            profiler,
            FrameProfiler::Chat
        );

        chatHUD.draw(
            profiler,
            ownedIds,
            screenW,
            screenH
        );
    }

    // --------------------------------------------------------
    // Player HUD, FPS + Ping, server realtime info
    // --------------------------------------------------------

    {
        FrameProfiler::ScopedTimer timer(
            profiler,
            FrameProfiler::PlayerHUD
        );

        playerHUD.draw(
            ownedIds,
            renderStates,
            screenW,
            screenH,
            fps,
            pingMs,
            serverSessionSeconds,
            serverInfoText,
            gameMode
        );
    }

    // --------------------------------------------------------
    // Finish legacy text renderer
    // --------------------------------------------------------

    {
        FrameProfiler::ScopedTimer timer(
            profiler,
            FrameProfiler::TextEnd
        );

        textRenderer.end(
            font,
            screenW,
            screenH,
            1.0f,
            1.0f,
            1.0f
        );
    }
}