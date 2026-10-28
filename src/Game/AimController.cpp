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

bool AimController::trySendFreshAim(const InputState& input, float, float, bool bypass)
{
    float ax, ay;
    camera.screenToWorld(input.mouseX, input.mouseY, ax, ay);
    return trySendWorldAim(ax, ay, bypass);
}

bool AimController::trySendWorldAim(float aimX, float aimY, bool bypassRateLimit)
{
    const double x = std::floor(aimX);
    const double y = std::floor(aimY);

    if (gameState.hasAimOld && x == gameState.aimXOld && y == gameState.aimYOld)
        return false;

    const auto now = std::chrono::steady_clock::now();
    if (!bypassRateLimit &&
        now - gameState.lastAimSendTime < std::chrono::milliseconds(4))
        return false;

    gameState.lastAimSendTime = now;
    gameState.aimXOld = static_cast<float>(x);   // лучше сделать поля double
    gameState.aimYOld = static_cast<float>(y);
    gameState.hasAimOld = true;

    network.sendAimPosition(x, y);
    return true;
}