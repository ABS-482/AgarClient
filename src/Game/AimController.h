#pragma once

#include "../Graphics/Camera.h"
#include "GameState.h"
#include "../Input/InputState.h"
#include "../Network/NetworkClient.h"

class AimController
{
public:
    AimController(
        Camera& camera,
        NetworkClient& network,
        GameState& gameState
    );

    bool trySendFreshAim(
        const InputState& input,
        float screenW,
        float screenH,
        bool bypassRateLimit
    );

private:
    Camera& camera;
    NetworkClient& network;
    GameState& gameState;
};