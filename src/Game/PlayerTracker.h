#pragma once

#include <cstdint>
#include <unordered_map>
#include <vector>

#include "GameState.h"
#include "RenderState.h"
#include "World.h"
#include "../Graphics/Camera.h"

class PlayerTracker
{
public:
    PlayerTracker(
        Camera& camera,
        GameState& gameState
    );

    void update(
        const WorldSnapshot& blobs,
        const std::vector<uint32_t>& ownedIds,
        const std::unordered_map<uint32_t, RenderState>& renderStates
    );

private:
    Camera& camera;
    GameState& gameState;
};