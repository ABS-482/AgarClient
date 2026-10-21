
#pragma once

#include <cstdint>
#include <unordered_map>
#include <vector>
#include <string>

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
#include "../Fonts/UiFonts.h"

class UserSkinDatabase;
class FrameProfiler;

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
        SkinManager& skinManager,
        UiFonts& uiFonts,
        UserSkinDatabase& userSkinDatabase
    );

    void draw(
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
    );

private:
    UIPanel& uiPanel;
    TextRenderer& textRenderer;
    Font& font;
    Camera& camera;
    World& world;
    GameState& gameState;

    UiFonts& uiFonts;

    LeaderboardHUD leaderboardHUD;
    PlayerHUD playerHUD;
    ChatHUD chatHUD;
};
