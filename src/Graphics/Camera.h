#pragma once

#include "../Game/Blob.h"

#include <cstdint>
#include <unordered_map>

class Camera
{
public:
    float x = 0.0f;
    float y = 0.0f;

    float zoom = 0.27f;        // фактический, применяемый зум (плавно едет к targetZoom)
    float targetZoom = 0.27f;

    float zoomScale = 1.0f;   // пользовательское предпочтение — то, что крутит колесо

    float targetX = 0.0f;
    float targetY = 0.0f;

    static constexpr float minZoomScale = 0.2f;
    static constexpr float maxZoomScale = 1.5f;

    // Подобрано эмпирически под ваш прошлый диапазон minZoom~0.05/maxZoom~0.4:
    // 0.2 * baseZoom ≈ 0.05, 1.5 * baseZoom ≈ 0.4 — компромиссное среднее.
    static constexpr float baseZoom = 0.27f;

    // Financial-strength safety net — на случай если baseZoom откалиброван неидеально.
    static constexpr float minZoom = 0.05f;
    static constexpr float maxZoom = 0.4f;

    void update(float deltaTime);
    void setManualTarget(float worldX, float worldY);
    void zoomBy(float wheelDelta);

    void screenToWorld(
        float screenX, float screenY,
        float screenWidth, float screenHeight,
        float& outWorldX, float& outWorldY
    ) const;

    void worldToScreen(
        float worldX, float worldY,
        float screenWidth, float screenHeight,
        float& outScreenX, float& outScreenY
    ) const;
};