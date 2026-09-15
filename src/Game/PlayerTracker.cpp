#include "PlayerTracker.h"

#include <algorithm>
#include <cmath>

PlayerTracker::PlayerTracker(
    Camera& camera,
    GameState& gameState
)
    : camera(camera),
    gameState(gameState)
{
}

void PlayerTracker::update(
    const WorldSnapshot& blobs,
    const std::vector<uint32_t>& ownedIds,
    const std::unordered_map<uint32_t, RenderState>& renderStates
)
{
    if (ownedIds.empty())
        return;

    float sumX = 0.0f;
    float sumY = 0.0f;
    int count = 0;

    float totalSize = 0.0f;

    gameState.currentScore = 0.0f;

    for (uint32_t id : ownedIds)
    {
        auto it = blobs->find(id);

        if (it != blobs->end())
        {
            auto rsIt = renderStates.find(id);

            float sx;
            float sy;
            float ssize;

            if (rsIt != renderStates.end())
            {
                sx = rsIt->second.x;
                sy = rsIt->second.y;
                ssize = rsIt->second.size;
            }
            else
            {
                sx = it->second.targetX;
                sy = it->second.targetY;
                ssize = it->second.targetSize;
            }

            sumX += sx;
            sumY += sy;
            totalSize += ssize;

            gameState.currentScore +=
                (ssize * ssize) / 100.0f;

            ++count;
        }
    }

    gameState.bestScoreSoFar =
        std::max(
            gameState.bestScoreSoFar,
            gameState.currentScore
        );

    if (totalSize > 0.0f)
    {
        float sizeFactor =
            std::pow(
                std::min(64.0f / totalSize, 1.0f),
                0.4f
            );

        camera.setSizeZoomFactor(sizeFactor);
    }

    if (count > 0)
    {
        float avgX = sumX / count;
        float avgY = sumY / count;

        camera.setManualTarget(avgX, avgY);
    }
}