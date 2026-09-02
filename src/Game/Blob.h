#pragma once

#include <cstdint>
#include <string>

enum class CellType : uint8_t
{
    Player = 0,
    Food = 1,
    Virus = 2,
    EjectedMass = 3 // предположительно — выброшенная масса
};

struct Blob
{
    uint32_t id = 0;

    // Текущая позиция/размер (для отрисовки без интерполяции пока).
    float x = 0.0f;
    float y = 0.0f;
    float size = 0.0f;

    // Цель последнего world update — пригодится позже для интерполяции.
    float targetX = 0.0f;
    float targetY = 0.0f;
    float targetSize = 0.0f;

    CellType cellType = CellType::Player;
    uint8_t colorIndex = 0;
    uint32_t skin = 0;
    uint32_t sticker = 0;
    uint16_t playerID = 0;
    std::string name;

    bool isVirus = false;
    uint8_t flags = 0;
};