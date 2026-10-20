#include "PlayerTracker.h"

#include <algorithm>

PlayerTracker::PlayerTracker(GameState& gameState)
    : gameState(gameState)
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

    gameState.currentScore = 0.0f;

    for (const uint32_t id : ownedIds)
    {
        const auto it = blobs->find(id);
        if (it == blobs->end())
            continue;

        float size;

        const auto rsIt = renderStates.find(id);
        if (rsIt != renderStates.end())
            size = rsIt->second.size;
        else
            size = it->second.targetSize;

        gameState.currentScore += (size * size) / 100.0f;
    }

    gameState.bestScoreSoFar =
        std::max(gameState.bestScoreSoFar, gameState.currentScore);
}