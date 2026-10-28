#include "Camera.h"
#include <algorithm>
#include <cmath>

long long Camera::nowMs()
{
    using namespace std::chrono;
    return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
}

static float softCap(float z) { return z > 7.0f ? 7.0f + (z - 7.0f) * 0.2f : z; }

static float spectatorZoom(GameMode m)
{
    if (m == GameMode::EatForSpeedArcade || m == GameMode::EatForSpeedRace) return 9.0f;
    if (m == GameMode::Arena || m == GameMode::Fatboy) return 16.0f;
    return 18.0f;
}

void Camera::onResize(float w, float h)
{
    width = w; height = h;
    const float gs = smallInterface ? 800.0f : 608.0f;
    const float s = (h / w > 1.0f) ? w / gs : h / gs;
    vw = (float)(int)(gs + (w - gs * s) / s);
    vh = (float)(int)(gs + (h - gs * s) / s);
    ppuX = w / vw;
    ppuY = h / vh;
}

bool Camera::update(float dt, const CameraInput& in, Vec2f* newTarget)
{
    bool setTarget = false;

    if (in.blobsCount > 0)
    {
        spectator = false;

        const long long elapsed = nowMs() - screenStart_;
        const float divisor = (elapsed < 2000) ? (2500.0f - (float)elapsed) / 500.0f : 1.0f;

        moveCameraPos(in, in.mode == GameMode::SnakeHero ? in.headPos : in.averagePos,
            dt / divisor);

        float z = 1.8f + in.totalRadius * 0.005f;
        if (in.mode == GameMode::SnakeHero)
            z = (0.0148f * in.headRadius + 1.0264f) * 1.1f;
        z = softCap(z);

        switch (in.mode) {
        case GameMode::EatForSpeedArcade:
        case GameMode::EatForSpeedRace: z *= 2.6f; break;
        case GameMode::SpaceInvaders:
        case GameMode::NewYearSpecial:  z += 3.0f; break;
        case GameMode::SelfFeed:
        case GameMode::Supernova:       z += 2.0f; break;
        default: break;
        }
        moveCameraZoom(in, z, dt);
    }
    else if (in.hasKiller)
    {
        float z = softCap(1.8f + in.killerRadius * 0.005f);
        if (spectator) {
            z = spectatorZoom(in.mode);
            if (newTarget) { *newTarget = in.killerPos; setTarget = true; }
        }
        moveCameraZoom(in, z, dt);
        moveCameraPos(in, in.killerPos, dt / 4.2f);
    }
    else if (spectator)
    {
        if (!in.drawingMode) moveCameraPos(in, in.inputTarget, dt / 5.2f);
        moveCameraZoom(in, spectatorZoom(in.mode), dt);
    }
    // иначе камера стоит на месте (как в Java)
    return setTarget;
}

void Camera::moveCameraPos(const CameraInput& in, Vec2f t, float k)
{
    const float off = in.pixelsEnabled ? 1000.0f : 0.0f;
    const float x = std::min(std::max(0.0f, t.x + off) - off, in.worldSize.x + off);
    const float y = std::min(std::max(0.0f, t.y + off) - off, in.worldSize.y + off);
    const float a = k * 30.0f;
    pos.x += (x - pos.x) * a;
    pos.y += (y - pos.y) * a;
}

void Camera::moveCameraZoom(const CameraInput& in, float target, float dt)
{
    target = std::clamp(target, 1.0f, 20.0f);
    if (in.smallInterface) target *= 0.76f;
    const float t = dt * 8.0f;
    zoom = zoom * (1.0f - t) + cameraScale * target * t;
}

void Camera::setCameraScale(float s) { cameraScale = std::clamp(s, 0.2f, 1.5f); }

void Camera::addCameraScale(float d, bool pixelsEnabled)
{
    if (pixelsEnabled && cameraScale < 0.2f) d *= 0.1f;
    cameraScale += d;
    if (cameraScale < 0.2f) cameraScale = 0.2f;
    else if (cameraScale > 1.5f && !zoomUnlocked) cameraScale = 1.5f;
    else if (cameraScale > 20.f && zoomUnlocked)   cameraScale = 20.f;
}

void Camera::onScroll(float amountY, bool pixelsEnabled)
{
    addCameraScale(-amountY * 0.3f, pixelsEnabled);
}

void Camera::toggleZoomUnlock()
{
    if (zoomUnlocked) { zoomUnlocked = false; setCameraScale(1.5f); }
    else zoomUnlocked = true;
}

void Camera::setCameraPos(const CameraInput& in, float x, float y)
{
    pos.x = std::min(std::max(0.0f, x), in.worldSize.x);
    pos.y = std::min(std::max(0.0f, y), in.worldSize.y);
}

// Y вниз (как в твоей старой версии). Эквивалент Java: там Y вверх и знак при Y обратный.
void Camera::screenToWorld(float sx, float sy, float& wx, float& wy) const
{
    wx = (sx - width * 0.5f) / scaleX() + pos.x;
    wy = (sy - height * 0.5f) / scaleY() + pos.y;
}

void Camera::worldToScreen(float wx, float wy, float& sx, float& sy) const
{
    sx = (wx - pos.x) * scaleX() + width * 0.5f;
    sy = (wy - pos.y) * scaleY() + height * 0.5f;
}