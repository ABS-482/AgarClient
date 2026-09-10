#pragma once

#include "../Game/Blob.h"

#include <cstdint>
#include <unordered_map>

class Camera
{
public:
    float x = 0.0f;
    float y = 0.0f;

    float zoom = 0.27f;
    float targetZoom = 0.27f;
    float zoomScale = 1.0f;
    float sizeZoomFactor = 1.0f;

    float targetX = 0.0f;
    float targetY = 0.0f;

    float minZoomScale = 0.2f;
    float maxZoomScale = 1.5f;
    float baseZoom = 0.27f;
    float minZoom = 0.05f;
    float maxZoom = 0.4f;


    bool hasBounds = false;
    float boundsMinX = 0.0f;
    float boundsMinY = 0.0f;
    float boundsMaxX = 0.0f;
    float boundsMaxY = 0.0f;

    void setBounds(float minX, float minY, float maxX, float maxY);

    void update(float deltaTime);
    void setManualTarget(float worldX, float worldY);
    void zoomBy(float wheelDelta);

    // Задаёт zoom мгновенно, минуя плавный lerp — удобно для стартовой
    // инициализации, когда доезжать до значения кадр за кадром не нужно.
    void setZoomImmediate(float desiredZoom);

    void snapTowardsTarget(float deltaTime, float halfLifeSeconds);

    void updateZoomOnly(float deltaTime);

    void setSizeZoomFactor(float factor) { sizeZoomFactor = factor; }

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

    void setZoomLimits(
        float minScale, float maxScale,
        float minAbsolute, float maxAbsolute
    );
};