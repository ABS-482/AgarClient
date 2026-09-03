
#include "Camera.h"

#include <algorithm>
#include <cmath>
#include <limits>

void Camera::fitToBlobs(const std::unordered_map<uint32_t, Blob>& blobs)
{
    if (blobs.empty() || hasManualTarget)
        return;

    float minX = std::numeric_limits<float>::max();
    float maxX = std::numeric_limits<float>::lowest();
    float minY = std::numeric_limits<float>::max();
    float maxY = std::numeric_limits<float>::lowest();

    for (const auto& [id, blob] : blobs)
    {
        minX = std::min(minX, blob.targetX);
        maxX = std::max(maxX, blob.targetX);
        minY = std::min(minY, blob.targetY);
        maxY = std::max(maxY, blob.targetY);
    }

    targetX = (minX + maxX) * 0.5f;
    targetY = (minY + maxY) * 0.5f;
}

void Camera::update(float deltaTime)
{
    float smoothing = 1.0f - std::exp(-10.0f * deltaTime);

    x += (targetX - x) * smoothing;
    y += (targetY - y) * smoothing;
}

void Camera::setManualTarget(float worldX, float worldY)
{
    targetX = worldX;
    targetY = worldY;
    hasManualTarget = true;
}

void Camera::zoomBy(float wheelDelta)
{
    float step = wheelDelta * 0.3f;

    if (zoom < minZoom * 4.0f) // порог замедления теперь пропорционален вашим реальным границам
    {
        step *= 0.1f;
    }

    zoom += step;
    zoom = std::clamp(zoom, minZoom, maxZoom);
}

void Camera::screenToWorld(
    float screenX, float screenY,
    float screenWidth, float screenHeight,
    float& outWorldX, float& outWorldY
) const
{
    outWorldX = (screenX - screenWidth * 0.5f) / zoom + x;
    outWorldY = (screenY - screenHeight * 0.5f) / zoom + y;
}

void Camera::worldToScreen(
    float worldX, float worldY,
    float screenWidth, float screenHeight,
    float& outScreenX, float& outScreenY
) const
{
    outScreenX = (worldX - x) * zoom + screenWidth * 0.5f;
    outScreenY = (worldY - y) * zoom + screenHeight * 0.5f;
}