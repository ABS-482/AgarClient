#pragma once

#include "../Game/Blob.h"

#include <cstdint>
#include <unordered_map>

class Camera
{
public:
    float x = 0.0f;
    float y = 0.0f;
    float zoom = 0.2f; // подберите под масштаб карты вашей игры

    float targetX = 0.0f;
    float targetY = 0.0f;

    // false — позиция авто-центрируется по всем сущностям.
    // true — после клика едет туда, куда кликнули.
    bool hasManualTarget = false;

    static constexpr float minZoom = 0.02f;
    static constexpr float maxZoom = 5.0f;

    void fitToBlobs(const std::unordered_map<uint32_t, Blob>& blobs);

    void update(float deltaTime);

    void setManualTarget(float worldX, float worldY);

    // Масштабирование колесом мыши: положительный delta — приблизить.
    void zoomBy(float wheelDelta);

    void screenToWorld(
        float screenX, float screenY,
        float screenWidth, float screenHeight,
        float& outWorldX, float& outWorldY
    ) const;
};