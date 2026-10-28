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

    // Переиспользуемый список клеток для агрегатов камеры.
    std::vector<CameraCell> cameraCells;

    // Текущая цель наблюдателя, сохраняется между кадрами.
    Vec2f spectatorTarget{};

    bool spectatorTargetInitialized = false;

    // Java хранит headPos/headRadius статически.
    // Если в текущем кадре голова не обновилась, сохраняем
    // предыдущее значение.
    Vec2f trackedHeadPos{};
    float trackedHeadRadius = 0.0f;
};