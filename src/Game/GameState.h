#pragma once

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
};