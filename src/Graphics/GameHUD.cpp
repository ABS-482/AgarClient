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
    GameState& gameState,
    Shader& chatSkinShader,
    SkinMesh& skinMesh,
    SkinManager& skinManager
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
    chatHUD(
        uiPanel,
        textRenderer,
        font,
        world,
        chatSkinShader,
        skinMesh,
        skinManager
    ),
    playerHUD(
        uiPanel,
        textRenderer,
        font,
        gameState
    )
{
}

void GameHUD::draw(
    const std::vector<uint32_t>& ownedIds,
    const std::unordered_map<uint32_t, RenderState>& renderStates,
    float screenW,
    float screenH,
    double fps
)
{
    textRenderer.begin();

    leaderboardHUD.draw(
        screenW,
        screenH
    );

    chatHUD.draw(
        ownedIds,
        screenW,
        screenH
    );

    playerHUD.draw(
        ownedIds,
        renderStates,
        screenW,
        screenH,
        fps
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