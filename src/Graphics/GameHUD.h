#pragma once

#include <cstdint>
#include <unordered_map>
#include <vector>

#include "Camera.h"
#include "Font.h"
#include "TextRenderer.h"
#include "UIPanel.h"

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
        GameState& gameState
    );

    void draw(
        const std::vector<uint32_t>& ownedIds,
        const std::unordered_map<uint32_t, RenderState>& renderStates,
        float screenW,
        float screenH
    );

private:
    UIPanel& uiPanel;
    TextRenderer& textRenderer;
    Font& font;
    Camera& camera;
    World& world;
    GameState& gameState;
};