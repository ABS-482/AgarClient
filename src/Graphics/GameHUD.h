#pragma once

#include <cstdint>
#include <unordered_map>
#include <vector>

#include "Camera.h"
#include "Font.h"
#include "TextRenderer.h"
#include "UIPanel.h"
#include "Shader.h"
#include "SkinManager.h"
#include "SkinMesh.h"
#include "../UI/HUD/LeaderboardHUD.h"
#include "../UI/HUD/PlayerHUD.h"
#include "../UI/HUD/ChatHUD.h"

#include "../Game/GameState.h"
#include "../Game/RenderState.h"
#include "../Game/World.h"

class GameHUD
{
public:
    GameHUD(
        UIPanel& uiPanel,
        TextRenderer& textRenderer,
        Font& font,
        Camera& camera,
        World& world,
        GameState& gameState,
        Shader& chatSkinShader,
        SkinMesh& skinMesh,
        SkinManager& skinManager
    );

    void draw(
        const std::vector<uint32_t>& ownedIds,
        const std::unordered_map<uint32_t, RenderState>& renderStates,
        float screenW,
        float screenH,
        double fps
    );

private:
    UIPanel& uiPanel;
    TextRenderer& textRenderer;
    Font& font;
    Camera& camera;
    World& world;
    GameState& gameState;

    LeaderboardHUD leaderboardHUD;
    PlayerHUD playerHUD;
    ChatHUD chatHUD;
};