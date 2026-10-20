#pragma once

#include "RenderState.h"
#include "World.h"

#include <cstdint>
#include <unordered_map>
#include <vector>

struct SplitPredictionCell
{
    float x = 0.0f;
    float y = 0.0f;
    float radius = 0.0f;
    float mass = 0.0f;

    // 0 = безопасно
    // 1 = простое пересечение
    // 2 = наша клетка может съесть чужую
    // 3 = чужая клетка может съесть нашу
    int dangerLevel = 0;

    // Предсказанная клетка попадёт на вирус
    // и должна лопнуть.
    bool hitsVirus = false;
};

struct SplitPredictionEnemy
{
    uint32_t id = 0;

    // 0 = безопасно
    // 1 = простое пересечение
    // 2 = наша клетка может съесть чужую
    // 3 = чужая клетка может съесть нашу
    int dangerLevel = 0;
};

struct SplitPredictionResult
{
    std::vector<SplitPredictionCell> cells;
    std::vector<SplitPredictionEnemy> enemies;
};

class SplitPrediction
{
public:
    SplitPredictionResult simulate(
        const WorldSnapshot& blobs,
        const std::vector<uint32_t>& ownedIds,
        const std::unordered_map<uint32_t, RenderState>& renderStates,
        float targetX,
        float targetY
    ) const;
};