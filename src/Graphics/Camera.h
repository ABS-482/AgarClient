#pragma once

#include "../Game/Blob.h"

#include <cstdint>
#include <unordered_map>

class Camera
{
public:
    float x = 0.0f;
    float y = 0.0f;

    float zoom = 1.5f;
    float targetZoom = 1.5f;
    float zoomScale = 1.0f;
    float sizeZoomFactor = 1.0f;

    float cameraScale = 1.0f;
    float targetJavaZoom = 1.5f;
    float targetX = 0.0f;
    float targetY = 0.0f;

    float minZoomScale = 0.2f;
    float maxZoomScale = 1.5f;
    float baseZoom = 1.5f;
    float minZoom = 1.0f;
    float maxZoom = 20.0f;

    float viewportWidth = 608.0f;
    float viewportHeight = 608.0f;

    bool hasBounds = false;
    float boundsMinX = 0.0f;
    float boundsMinY = 0.0f;
    float boundsMaxX = 0.0f;
    float boundsMaxY = 0.0f;

    void setViewport(float width, float height);

    void setBounds(float minX, float minY, float maxX, float maxY);

    void update(float deltaTime);
    void setManualTarget(float worldX, float worldY);
    void zoomBy(float wheelDelta);
    void setTargetJavaZoom(float value);
    void updateJavaZoom(float deltaTime);

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