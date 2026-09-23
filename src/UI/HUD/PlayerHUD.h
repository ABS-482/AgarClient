#pragma once

#include <cstdint>
#include <unordered_map>
#include <vector>

#include "../../Game/RenderState.h"

class UIPanel;
class TextRenderer;
class Font;
class GameState;

class PlayerHUD
{
public:
    PlayerHUD(
        UIPanel& uiPanel,
        TextRenderer& textRenderer,
        Font& font,
        GameState& gameState
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
    GameState& gameState;
};