#pragma once
#include <chrono>
#include <vector>

struct Vec2f { float x = 0, y = 0; };

enum class GameMode {
    Other, Arena, Fatboy, SnakeHero, EatForSpeedArcade, EatForSpeedRace,
    SpaceInvaders, SelfFeed, Supernova, NewYearSpecial
};

struct CameraCell { float x = 0, y = 0, size = 0; };   // size = радиус

// Всё, что камера берёт из состояния игры. Заполняется каждый кадр ПОСЛЕ act() клеток.
struct CameraInput
{
    int   blobsCount = 0;
    Vec2f averagePos, headPos;          // headPos/headRadius только для SnakeHero
    float headRadius = 0, totalRadius = 0;

    bool  hasKiller = false;
    Vec2f killerPos;
    float killerRadius = 0;

    Vec2f inputTarget;                  // мировая цель курсора (для спектатора)
    bool  drawingMode = false;
    bool  smallInterface = false;
    bool  pixelsEnabled = false;
    GameMode mode = GameMode::Other;
    Vec2f worldSize;                    // СЫРЫЕ width/height границы (maxX/maxY)

    // Быстрый способ заполнить агрегаты из старого вектора клеток
    void fromCells(const std::vector<CameraCell>& cells)
    {
        blobsCount = (int)cells.size();
        averagePos = {}; totalRadius = 0;
        for (auto& c : cells) { averagePos.x += c.x; averagePos.y += c.y; totalRadius += c.size; }
        if (blobsCount > 0) { averagePos.x /= blobsCount; averagePos.y /= blobsCount; }
    }
};

class Camera
{
public:
    // --- окно / viewport (Graphics.updateSize) ---
    float width = 1920, height = 1080;  // пиксели окна
    float vw = 608, vh = 608;           // viewportWidth/Height
    float ppuX = 1, ppuY = 1;
    bool  smallInterface = false;

    // --- состояние (GameScreen) ---
    Vec2f pos{ 0, 0 };                    // начальная позиция (0,0)
    float zoom = 1.5f;                  // множитель видимой области
    float cameraScale = 1.0f;           // пользовательский зум
    bool  spectator = false;
    bool  zoomUnlocked = false;         // Inputs.isZoom (Shift)

    void onResize(float w, float h);            // вызывать и при смене smallInterface
    void onShow() { screenStart_ = nowMs(); }
    void resetStartTime() { screenStart_ = nowMs(); }

    // Возвращает true, если нужно выставить inputs.target = *newTarget
    bool update(float dt, const CameraInput& in, Vec2f* newTarget);

    // ввод
    void onScroll(float amountY, bool pixelsEnabled = false);  // >0 = колесо вниз = отдалить
    void addCameraScale(float d, bool pixelsEnabled = false);
    void setCameraScale(float s);
    void toggleZoomUnlock();                                   // Shift
    void setCameraPos(const CameraInput& in, float x, float y);

    // рендер / ввод
    float scaleX() const { return ppuX / zoom; }
    float scaleY() const { return ppuY / zoom; }
    void screenToWorld(float sx, float sy, float& wx, float& wy) const;
    void worldToScreen(float wx, float wy, float& sx, float& sy) const;

private:
    long long screenStart_ = 0;
    static long long nowMs();
    void moveCameraPos(const CameraInput& in, Vec2f target, float k);
    void moveCameraZoom(const CameraInput& in, float target, float dt);
};