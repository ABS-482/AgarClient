
#include "Camera.h"

#include <algorithm>
#include <cmath>
#include <limits>

void Camera::update(float deltaTime)
{
    float smoothing = 1.0f - std::exp(-10.0f * deltaTime);

    x += (targetX - x) * smoothing;
    y += (targetY - y) * smoothing;

    targetZoom = zoomScale * baseZoom;

    // Тот же линейный lerp, что и в Java: dt * 8, зажатый в 0..1
    // на случай очень длинных кадров (лаг-спайков).
    float zoomSmoothing = std::clamp(deltaTime * 8.0f, 0.0f, 1.0f);
    zoom += (targetZoom - zoom) * zoomSmoothing;

    zoom = std::clamp(zoom, minZoom, maxZoom);
}

void Camera::setManualTarget(float worldX, float worldY)
{
    targetX = worldX;
    targetY = worldY;
}

void Camera::zoomBy(float wheelDelta)
{
    zoomScale *= std::pow(1.15f, wheelDelta);
    zoomScale = std::clamp(zoomScale, minZoomScale, maxZoomScale);
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