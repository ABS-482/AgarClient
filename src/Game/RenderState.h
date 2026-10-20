#pragma once

#include "World.h"

#include <chrono>
#include <cstdint>

// Параметры текущей adaptive-интерполяции.
// На этом шаге оставляем их, чтобы переход на JS-модель
// выполнялся постепенно и проект продолжал собираться.
struct InterpolationSettings
{
    // Длительность lerp = измеренный интервал апдейтов * margin.
    float ownMargin = 1.25f;
    float otherMargin = 1.75f;

    // Экстраполяция своих клеток.
    float ownLead = 0.03f;

    // Ограничение экстраполяции.
    float maxLeadInSizes = 0.5f;

    // Сглаживание размера.
    float sizeTau = 0.06f;

    // Границы длительности lerp.
    float minDuration = 0.02f;
    float maxDuration = 0.20f;

    // Ограничение deltaTime после фриза.
    float maxDeltaTime = 0.10f;
};

struct RenderState
{
    // --------------------------------------------------------
    // Текущее отображаемое состояние.
    //
    // Аналог JS:
    // this.x
    // this.y
    // this.size
    // --------------------------------------------------------

    float x = 0.0f;
    float y = 0.0f;
    float size = 0.0f;

    // --------------------------------------------------------
    // JS interpolation state.
    //
    // Аналог:
    // this.ox
    // this.oy
    // this.oSize
    // --------------------------------------------------------

    float ox = 0.0f;
    float oy = 0.0f;
    float oSize = 0.0f;

    // --------------------------------------------------------
    // Новое состояние.
    //
    // Аналог:
    // this.nx
    // this.ny
    // this.nSize
    // --------------------------------------------------------

    float nx = 0.0f;
    float ny = 0.0f;
    float nSize = 0.0f;

    // --------------------------------------------------------
    // Аналог JS:
    // this.updateTime
    // --------------------------------------------------------

    std::chrono::steady_clock::time_point updateTime{};

    // --------------------------------------------------------
    // Старая adaptive-система.
    //
    // Пока оставляем для совместимости.
    // На следующих шагах она будет удалена.
    // --------------------------------------------------------

    float serverMoveX = 0.0f;
    float serverMoveY = 0.0f;

    float serverPosX = 0.0f;
    float serverPosY = 0.0f;

    float moveTimer = 0.0f;

    float rawX = 0.0f;
    float rawY = 0.0f;

    float vx = 0.0f;
    float vy = 0.0f;

    float updateInterval = 0.04f;
    float lerpDuration = 0.05f;

    // --------------------------------------------------------
    // Размер.
    // --------------------------------------------------------

    float targetSize = 0.0f;

    // x/y/size уже используются выше.
    // Здесь больше отдельного size не объявляем.

    float radiusDelta = 0.0f;

    // --------------------------------------------------------
    // Служебные данные.
    // --------------------------------------------------------

    std::chrono::steady_clock::time_point lastSeenUpdate{};

    CellType lastCellType = CellType::Food;

    bool initialized = false;
    bool isLeading = false;

    // --------------------------------------------------------
    // JS updatePos()
    //
    // Полный аналог:
    //
    // var ratio =
    //     (timestampLastDraw - this.updateTime) / 120;
    //
    // this.x =
    //     ratio * (this.nx - this.ox) + this.ox;
    //
    // this.y =
    //     ratio * (this.ny - this.oy) + this.oy;
    //
    // this.size =
    //     ratio * (this.nSize - this.oSize) + this.oSize;
    // --------------------------------------------------------

    float updatePos(
        std::chrono::steady_clock::time_point timestampLastDraw
    )
    {
        float ratio =
            std::chrono::duration<float, std::milli>(
                timestampLastDraw - updateTime
            ).count() / 120.0f;

        if (ratio < 0.0f)
        {
            ratio = 0.0f;
        }
        else if (ratio > 1.0f)
        {
            ratio = 1.0f;
        }

        x =
            ratio * (nx - ox) + ox;

        y =
            ratio * (ny - oy) + oy;

        size =
            ratio * (nSize - oSize) + oSize;

        return ratio;
    }
};

struct DrawEntry
{
    uint32_t id;
    const Blob* blob;
    RenderState* rs;
};