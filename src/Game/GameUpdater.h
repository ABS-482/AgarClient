#pragma once

#include <cstdint>
#include <vector>

#include "../Graphics/Camera.h"
#include "../Graphics/SkinManager.h"
#include "../Input/InputState.h"
#include "../Network/NetworkClient.h"
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
};