#pragma once

#include "World.h"

#include <chrono>
#include <cstdint>

struct RenderState
{
    float prevX = 0.0f;
    float prevY = 0.0f;
    float prevSize = 0.0f;

    float x = 0.0f;
    float y = 0.0f;
    float size = 0.0f;

    std::chrono::steady_clock::time_point lastSeenUpdate{};
    CellType lastCellType = CellType::Food;
    bool initialized = false;
};

struct DrawEntry
{
    uint32_t id;
    const Blob* blob;
    RenderState* rs;
};