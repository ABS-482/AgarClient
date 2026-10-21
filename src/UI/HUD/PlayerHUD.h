#pragma once

#include <cstdint>
#include <unordered_map>
#include <vector>
#include <string>

#include "../../Game/RenderState.h"

class UIPanel;
class TextRenderer;
class Font;
class GameState;
class UiFonts;

class PlayerHUD
{
public:
    PlayerHUD(
        UIPanel& uiPanel,
        GameState& gameState,
        UiFonts& uiFonts
    );

    void draw(
        const std::vector<uint32_t>& ownedIds,
        const std::unordered_map<uint32_t, RenderState>& renderStates,
        float screenW,
        float screenH,
        double fps,
        int pingMs,
        std::int64_t serverSessionSeconds,
        const std::string& serverInfoText
    );

private:
    UIPanel& uiPanel;
    UiFonts& uiFonts;
    GameState& gameState;
};