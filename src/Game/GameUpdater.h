#pragma once

#include <cstdint>
#include <vector>
#include <unordered_map>

#include "../Graphics/Camera.h"
#include "../Graphics/SkinManager.h"
#include "../Input/InputState.h"
#include "../Network/NetworkClient.h"
#include "../Game/RenderState.h"
#include "GameState.h"
#include "World.h"

class GameUpdater
{
public:
    GameUpdater(
        Camera& camera,
        NetworkClient& network,
        SkinManager& skinManager,
        World& world,
        GameState& gameState
    );

    void update(
        const InputState& input,
        const std::vector<uint32_t>& ownedIds,
        const std::unordered_map<uint32_t, RenderState>& renderStates,
        float deltaTime,
        float screenW,
        float screenH,
        bool menuOpen
    );

private:
    Camera& camera;
    NetworkClient& network;
    SkinManager& skinManager;
    World& world;
    GameState& gameState;

    // Переиспользуемый буфер, чтобы не выделять память каждый кадр.
    std::vector<CameraCell> cameraCells;
};