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
    UiFont& chatFont,
    UiTextRenderer& chatTextRenderer,
    UserSkinDatabase& userSkinDatabase
)
    : uiPanel(uiPanel),
    textRenderer(textRenderer),
    font(font),
    camera(camera),
    world(world),
    gameState(gameState),
    leaderboardHUD(
        uiPanel,
        textRenderer,
        font,
        world
    ),
    playerHUD(
        uiPanel,
        textRenderer,
        font,
        gameState
    ),
    chatHUD(
        uiPanel,
        textRenderer,
        font,
        world,
        chatSkinShader,
        skinMesh,
        skinManager,
        chatFont,
        chatTextRenderer,
        userSkinDatabase
    ),
    chatFont(chatFont),
    chatTextRenderer(chatTextRenderer)
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
    const std::string& serverInfoText
)
{
    textRenderer.begin();

    {
        FrameProfiler::ScopedTimer timer(
            profiler,
            FrameProfiler::Leaderboard
        );

        leaderboardHUD.draw(ownedIds, screenW, screenH);
    }

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
            serverInfoText
        );
    }

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