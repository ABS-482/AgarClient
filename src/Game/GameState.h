#pragma once

#include "../Graphics/Camera.h"

#include <chrono>

struct GameState
{
    bool mapCentered = false;

    float aimXOld = 0.0f;
    float aimYOld = 0.0f;

    float bestScoreSoFar = 0.0f;
    float currentScore = 0.0f;

    bool hasAimOld = false;

    std::chrono::steady_clock::time_point lastAimSendTime{};
    std::chrono::steady_clock::time_point lastMacroShotTime{};
    std::chrono::steady_clock::time_point lastFreshAimSendTime{};

    // --------------------------------------------------------
    // Параметры камеры.
    // Эти значения необходимо обновлять из настроек игры
    // и параметров выбранного сервера.
    // --------------------------------------------------------

    GameMode cameraMode = GameMode::Other;

    bool pixelsEnabled = false;
    bool drawingMode = false;

    // Состояние клетки-убийцы, если оно реализовано в игре.
    bool hasKiller = false;
    Vec2f killerPos{};
    float killerRadius = 0.0f;
};