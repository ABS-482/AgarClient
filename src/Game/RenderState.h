#pragma once

#include "World.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>

// Сохраняем для совместимости со старым кодом.
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
    // Текущее отображаемое состояние.
    float x = 0.0f;
    float y = 0.0f;
    float size = 0.0f;

    // Начальная позиция текущего движения.
    float ox = 0.0f;
    float oy = 0.0f;
    float oSize = 0.0f;

    // Целевая позиция.
    float nx = 0.0f;
    float ny = 0.0f;
    float nSize = 0.0f;

    // Оставлено для совместимости и отслеживания обновлений.
    std::chrono::steady_clock::time_point updateTime{};

    // Старые поля adaptive-системы.
    float serverMoveX = 0.0f;
    float serverMoveY = 0.0f;

    float serverPosX = 0.0f;
    float serverPosY = 0.0f;

    float moveTimer = 0.0f;

    float rawX = 0.0f;
    float rawY = 0.0f;

    float vx = 0.0f;
    float vy = 0.0f;

    float updateInterval = 0.04f;
    float lerpDuration = 0.05f;

    // Таймер движения, аналог Java lastUpdateTime.
    // Увеличивается на deltaTime каждого кадра.
    float positionElapsed = 0.0f;

    // Целевой размер и прогресс его сглаживания.
    float targetSize = 0.0f;
    float radiusDelta = 0.8f;

    // Отметка последнего сетевого обновления.
    std::chrono::steady_clock::time_point lastSeenUpdate{};

    CellType lastCellType = CellType::Food;

    bool initialized = false;
    bool isLeading = false;

    // Аналог Java Blob.setRadius().
    void setTargetSize(float newTargetSize)
    {
        if (targetSize == newTargetSize)
            return;

        radiusDelta =
            radiusDelta < 1.0f
            ? radiusDelta / 2.0f
            : 0.0f;

        targetSize = newTargetSize;
    }

    // Интерполяция от текущей позиции к последней серверной цели.
    float updatePos(float deltaTime)
    {
        positionElapsed += deltaTime;

        const float duration = std::max(lerpDuration, 0.001f);

        const float ratio = std::clamp(
            positionElapsed / duration,
            0.0f,
            1.0f
        );

        x = ox + (nx - ox) * ratio;
        y = oy + (ny - oy) * ratio;

        return ratio;
    }

    // Отдельное изменение радиуса, как в Java Blob.act(dt).
    void updateRadius(float deltaTime)
    {
        if (deltaTime <= 0.0f || size == targetSize)
            return;

        radiusDelta = std::min(
            radiusDelta + deltaTime * 3.0f,
            1.0f
        );

        constexpr float halfPi =
            1.57079632679489661923f;

        // LibGDX Interpolation.sineIn.
        const float alpha =
            1.0f - std::cos(radiusDelta * halfPi);

        float nextSize =
            size + (targetSize - size) * alpha;

        if (std::abs(
            std::abs(size) - std::abs(targetSize)
        ) < 0.01f)
        {
            nextSize = targetSize;
        }

        size = nextSize;
    }
};

struct DrawEntry
{
    uint32_t id;
    const Blob* blob;
    RenderState* rs;
};