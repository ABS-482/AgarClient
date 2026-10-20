#include "Camera.h"

#include <algorithm>
#include <cmath>

namespace
{
    constexpr float kMaxDt = 0.1f;   // после фриза не телепортируем камеру

    // JS считает «x = (x*(1-f) + target*f)» за КАДР при ~60 FPS.
    // Это даёт тот же результат на 60 FPS и такой же по времени на любом другом.
    inline float frameAlpha(float perFrame, float dt)
    {
        return 1.0f - std::pow(1.0f - perFrame, dt * 60.0f);
    }
}

void Camera::onResize(float newWidth, float newHeight)
{
    width = newWidth;
    height = newHeight;
}

// Math.max(height / 1080, width / 1920) * zoom
float Camera::unitRatio() const
{
    return std::max(height / 1080.0f, width / 1920.0f) * zoom;
}

void Camera::updateRatio(const std::vector<CameraCell>& cells, float dt)
{
    if (cells.empty())
        return;

    float size = 0.0f;
    for (const auto& c : cells)
        size += c.size;

    size = std::pow(std::min(64.0f / size, 1.0f), 0.4f) * unitRatio();

    // ratio = (9 * ratio + size) / 10;
    ratio += (size - ratio) * frameAlpha(0.1f, dt);
}

void Camera::update(const std::vector<CameraCell>& cells, float dt)
{
    dt = std::clamp(dt, 0.0f, kMaxDt);

    if (!cells.empty())
    {
        updateRatio(cells, dt);

        float sx = 0.0f, sy = 0.0f;
        for (const auto& c : cells)
        {
            sx += c.x / cells.size();
            sy += c.y / cells.size();
        }
        middleX = sx;
        middleY = sy;
            

        chunk = ratio;

        // offsetX = (offsetX + middleX) / 2;
        const float a = frameAlpha(0.5f, dt);
        offsetX += (middleX - offsetX) * a;
        offsetY += (middleY - offsetY) * a;
    }
    else
    {
        // offsetX = (29 * offsetX + middleX) / 30;
        const float a = frameAlpha(1.0f / 30.0f, dt);
        offsetX += (middleX - offsetX) * a;
        offsetY += (middleY - offsetY) * a;

        // ratio = (9 * ratio + chunk * unitRatio()) / 10;
        ratio += (chunk * unitRatio() - ratio) * frameAlpha(0.1f, dt);
    }
}

void Camera::zoomBy(float wheelDelta)
{
    zoom *= std::pow(0.9f, wheelDelta / -120.0f);

    if (!isSpectating)
    {
        clampZoomInGame();
    }
    else {
        if (zoom < 0.02f)          zoom = 0.02f;
        if (zoom > 3.0f / ratio)   zoom = 3.0f / ratio;
    }
}

void Camera::clampZoomInGame()
{
    const float floor = 0.6f;

    if (floor > zoom)         zoom = floor;
    if (zoom > 4.0f / ratio)  zoom = 4.0f / ratio;
}

// Типичный agar-трансформ: translate(w/2, h/2) → scale(ratio) → translate(-offset)
void Camera::screenToWorld(float screenX, float screenY,
    float& outWorldX, float& outWorldY) const
{
    outWorldX = (screenX - width * 0.5f) / ratio + offsetX;
    outWorldY = (screenY - height * 0.5f) / ratio + offsetY;
}

void Camera::worldToScreen(float worldX, float worldY,
    float& outScreenX, float& outScreenY) const
{
    outScreenX = (worldX - offsetX) * ratio + width * 0.5f;
    outScreenY = (worldY - offsetY) * ratio + height * 0.5f;
}