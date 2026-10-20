#pragma once

#include <vector>

// Минимум, что камере нужно знать про клетки игрока.
// Подставь сюда поля из своего Blob (x, y, size).
struct CameraCell
{
    float x = 0.0f;
    float y = 0.0f;
    float size = 0.0f;
};

class Camera
{
public:
    // --- Поля 1:1 с JS ---
    float width = 1920.0f;   // win.innerWidth
    float height = 1080.0f;   // win.innerHeight

    float zoom = 1.0f;     // пользовательский зум (колесо мыши)
    float ratio = 1.0f;     // итоговый масштаб рисования
    float chunk = 1.0f;     // последний ratio, пока игрок жив

    float offsetX = 0.0f;     // позиция камеры (сглаженная)
    float offsetY = 0.0f;
    float middleX = 0.0f;     // целевая точка (центр масс клеток)
    float middleY = 0.0f;

    bool isSpectating = false;

    // --- Функции 1:1 с JS ---
    void  onResize(float newWidth, float newHeight);
    float unitRatio() const;
    void  updateRatio(const std::vector<CameraCell>& cells, float dt);

    // Камерная часть draw(): вызывать каждый кадр ПОСЛЕ updatePos() клеток.
    void  update(const std::vector<CameraCell>& cells, float dt);

    // Колесо мыши (замени на формулу из JS, если там другая)
    void  zoomBy(float wheelDelta);

    void screenToWorld(float screenX, float screenY,
        float& outWorldX, float& outWorldY) const;
    void worldToScreen(float worldX, float worldY,
        float& outScreenX, float& outScreenY) const;
private:
    void clampZoomInGame();
};