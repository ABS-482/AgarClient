#pragma once

#include "World.h"

#include <algorithm>
#include <chrono>
#include <cstdint>

struct InterpolationSettings
{
    float ownMargin = 1.25f;
    float otherMargin = 1.75f;
    float ownLead = 0.03f;
    float maxLeadInSizes = 0.5f;
    float sizeTau = 0.06f;
    float minDuration = 0.02f;
    float maxDuration = 0.20f;
    float maxDeltaTime = 0.10f;
};

struct RenderState
{
    // Отображаемое состояние (JS: x, y, size)
    float x = 0.0f, y = 0.0f, size = 0.0f;

    // Старт отрезка (JS: ox, oy, oSize)
    float ox = 0.0f, oy = 0.0f, oSize = 0.0f;

    // Цель (JS: nx, ny, nSize)
    float nx = 0.0f, ny = 0.0f, nSize = 0.0f;

    // JS: updateTime
    std::chrono::steady_clock::time_point updateTime{};

    // ---- Поля для совместимости, на расчёт не влияют ----
    float serverMoveX = 0.0f, serverMoveY = 0.0f;
    float serverPosX = 0.0f, serverPosY = 0.0f;
    float moveTimer = 0.0f;
    float rawX = 0.0f, rawY = 0.0f;
    float vx = 0.0f, vy = 0.0f;
    float updateInterval = 0.04f;
    float lerpDuration = 0.12f;
    float positionElapsed = 0.0f;
    float targetSize = 0.0f;
    float radiusDelta = 0.8f;

    std::chrono::steady_clock::time_point lastSeenUpdate{};
    CellType lastCellType = CellType::Food;
    bool initialized = false;
    bool isLeading = false;

    // Раньше менял radiusDelta, теперь только запоминает цель.
    void setTargetSize(float newTargetSize) { targetSize = newTargetSize; }

    // JS updatePos(): ratio = (now - updateTime) / 120
    float updatePos(std::chrono::steady_clock::time_point now)
    {
        float ratio = std::chrono::duration<float, std::milli>(now - updateTime).count() / 120.0f;
        ratio = std::clamp(ratio, 0.0f, 1.0f);

        x = ratio * (nx - ox) + ox;
        y = ratio * (ny - oy) + oy;
        size = ratio * (nSize - oSize) + oSize;
        return ratio;
    }

    // Заглушка: размер теперь считается в updatePos.
    void updateRadius(float) {}
};

struct DrawEntry
{
    uint32_t id;
    const Blob* blob;
    RenderState* rs;
};