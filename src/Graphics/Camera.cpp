#include "Camera.h"

#include <algorithm>
#include <cmath>
#include <limits>

void Camera::setBounds(float minX, float minY, float maxX, float maxY)
{
    boundsMinX = minX;
    boundsMinY = minY;
    boundsMaxX = maxX;
    boundsMaxY = maxY;
    hasBounds = true;
}

void Camera::update(float deltaTime)
{
    float smoothing = 1.0f - std::exp(-10.0f * deltaTime);

    x += (targetX - x) * smoothing;
    y += (targetY - y) * smoothing;

    if (hasBounds)
    {
        x = std::clamp(x, boundsMinX, boundsMaxX);
        y = std::clamp(y, boundsMinY, boundsMaxY);
    }

    targetZoom = zoomScale * baseZoom;

    float zoomSmoothing = std::clamp(deltaTime * 8.0f, 0.0f, 1.0f);
    zoom += (targetZoom - zoom) * zoomSmoothing;

    zoom = std::clamp(zoom, minZoom, maxZoom);
}

void Camera::zoomBy(float wheelDelta)
{
    zoomScale *= std::pow(1.15f, wheelDelta);
    zoomScale = std::clamp(zoomScale, minZoomScale, maxZoomScale);
}

void Camera::setManualTarget(float worldX, float worldY)
{
    if (hasBounds)
    {
        worldX = std::clamp(worldX, boundsMinX, boundsMaxX);
        worldY = std::clamp(worldY, boundsMinY, boundsMaxY);
    }

    targetX = worldX;
    targetY = worldY;
}

void Camera::setZoomImmediate(float desiredZoom)
{
    desiredZoom = std::clamp(desiredZoom, minZoom, maxZoom);

    zoomScale = std::clamp(desiredZoom / baseZoom, minZoomScale, maxZoomScale);
    targetZoom = zoomScale * baseZoom;
    zoom = targetZoom;
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

void Camera::snapTowardsTarget(float deltaTime, float halfLifeSeconds)
{
    // halfLifeSeconds — время, за которое расстояние до цели
    // сокращается вдвое, НЕЗАВИСИМО от FPS.
    float smoothing = 1.0f - std::pow(0.5f, deltaTime / halfLifeSeconds);

    x = x + (targetX - x) * smoothing;
    y = y + (targetY - y) * smoothing;

    if (hasBounds)
    {
        x = std::clamp(x, boundsMinX, boundsMaxX);
        y = std::clamp(y, boundsMinY, boundsMaxY);
    }
}

void Camera::updateZoomOnly(float deltaTime)
{
    float effectiveDivisor = baseZoom * std::max(sizeZoomFactor, 0.0001f);

    float dynamicMinScale = std::max(minZoomScale, minZoom / effectiveDivisor);
    float dynamicMaxScale = std::min(maxZoomScale, maxZoom / effectiveDivisor);

    if (dynamicMinScale > dynamicMaxScale)
    {
        std::swap(dynamicMinScale, dynamicMaxScale);
    }

    zoomScale = std::clamp(zoomScale, dynamicMinScale, dynamicMaxScale);

    targetZoom = zoomScale * baseZoom * sizeZoomFactor;

    float zoomSmoothing = std::clamp(deltaTime * 8.0f, 0.0f, 1.0f);
    zoom += (targetZoom - zoom) * zoomSmoothing;

    zoom = std::clamp(zoom, minZoom, maxZoom);
}

void Camera::setZoomLimits(
    float minScale, float maxScale,
    float minAbsolute, float maxAbsolute
)
{
    minZoomScale = minScale;
    maxZoomScale = maxScale;
    minZoom = minAbsolute;
    maxZoom = maxAbsolute;

    zoomScale = std::clamp(zoomScale, minZoomScale, maxZoomScale);
}