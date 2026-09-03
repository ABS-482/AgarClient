#pragma once

#include <chrono>
#include <cstdint>
#include <string>

enum class CellType : uint8_t
{
    Player = 0,
    Food = 1,
    Virus = 2,
    EjectedMass = 3
};

struct Blob
{
    uint32_t id = 0;

    float targetX = 0.0f;
    float targetY = 0.0f;
    float targetSize = 0.0f;

    std::chrono::steady_clock::time_point lastUpdateTime =
        std::chrono::steady_clock::now();

    CellType cellType = CellType::Player;
    uint8_t colorIndex = 0;
    uint32_t skin = 0;
    uint32_t sticker = 0;
    uint16_t playerID = 0;
    std::string name;

    bool isVirus = false;
    uint8_t flags = 0;
};