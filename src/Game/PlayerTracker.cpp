#include "PlayerTracker.h"

#include <algorithm>
#include <cmath>

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
    gameState.currentScore = 0.0f;

    if (ownedIds.empty())
        return;

    for (const uint32_t id : ownedIds)
    {
        const auto it = blobs->find(id);
        if (it == blobs->end())
            continue;

        // Java использует targetRadius, а не интерполированный
        // радиус отрисовки.
        const float size = it->second.targetSize;

        gameState.currentScore +=
            std::ceil((size * size) / 100.0f);
    }

    gameState.bestScoreSoFar =
        std::max(
            gameState.bestScoreSoFar,
            gameState.currentScore
        );
}