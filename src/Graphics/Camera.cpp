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

void Camera::setViewport(float width, float height)
{
    viewportWidth = width;
    viewportHeight = height;
}

void Camera::zoomBy(float wheelDelta)
{
    cameraScale *=
        std::pow(1.15f, wheelDelta);

    cameraScale =
        std::clamp(
            cameraScale,
            0.2f,
            1.5f
        );
}

void Camera::setTargetJavaZoom(float value)
{
    targetJavaZoom =
        std::clamp(
            value,
            1.0f,
            20.0f
        );
}

void Camera::updateJavaZoom(float deltaTime)
{
    float alpha =
        std::clamp(
            deltaTime * 8.0f,
            0.0f,
            1.0f
        );

    targetZoom =
        cameraScale * targetJavaZoom;

    zoom =
        zoom * (1.0f - alpha) +
        targetZoom * alpha;

    zoom =
        std::clamp(
            zoom,
            0.2f,
            30.0f
        );
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
    desiredZoom =
        std::clamp(
            desiredZoom,
            minZoom,
            maxZoom
        );

    zoom = desiredZoom;
    targetZoom = desiredZoom;
    cameraScale = 1.0f;
}

void Camera::screenToWorld(
    float screenX, float screenY,
    float screenWidth, float screenHeight,
    float& outWorldX, float& outWorldY
) const
{
    const float scaleX =
        screenWidth / viewportWidth;

    const float scaleY =
        screenHeight / viewportHeight;

    outWorldX =
        x +
        (screenX - screenWidth * 0.5f) *
        zoom /
        scaleX;

    outWorldY =
        y -
        (screenY - screenHeight * 0.5f) *
        zoom /
        scaleY;
}

void Camera::worldToScreen(
    float worldX, float worldY,
    float screenWidth, float screenHeight,
    float& outScreenX, float& outScreenY
) const
{
    const float scaleX =
        screenWidth / viewportWidth;

    const float scaleY =
        screenHeight / viewportHeight;

    outScreenX =
        screenWidth * 0.5f +
        (worldX - x) *
        scaleX /
        zoom;

    outScreenY =
        screenHeight * 0.5f -
        (worldY - y) *
        scaleY /
        zoom;
}

void Camera::snapTowardsTarget(float deltaTime, float halfLifeSeconds)
{
    float smoothing =
        std::clamp(deltaTime * 30.0f, 0.0f, 1.0f);

    x += (targetX - x) * smoothing;
    y += (targetY - y) * smoothing;

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