#include "AimController.h"

#include <chrono>
#include <cmath>

AimController::AimController(
    Camera& camera,
    NetworkClient& network,
    GameState& gameState
)
    : camera(camera),
    network(network),
    gameState(gameState)
{
}

bool AimController::trySendFreshAim(
    const InputState& input,
    float screenW,
    float screenH,
    bool bypassRateLimit
)
{
    float aimX;
    float aimY;

    camera.screenToWorld(
        input.mouseX,
        input.mouseY,
        screenW,
        screenH,
        aimX,
        aimY
    );

    const bool changedEnough =
        !gameState.hasAimOld ||
        std::abs(gameState.aimXOld - aimX) >= 0.01f ||
        std::abs(gameState.aimYOld - aimY) >= 0.01f;

    if (!changedEnough)
        return false;

    const auto now =
        std::chrono::steady_clock::now();

    if (
        !bypassRateLimit &&
        now - gameState.lastAimSendTime <
        std::chrono::milliseconds(4)
        )
    {
        return false;
    }

    gameState.lastAimSendTime = now;
    gameState.aimXOld = aimX;
    gameState.aimYOld = aimY;
    gameState.hasAimOld = true;

    network.sendAimPosition(
        std::floor(aimX),
        std::floor(aimY)
    );

    return true;
}