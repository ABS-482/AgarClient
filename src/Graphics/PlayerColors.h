#pragma once

#include <cstdint>

struct RGB
{
    uint8_t r;
    uint8_t g;
    uint8_t b;
};

RGB getPlayerColor(uint8_t colorIndex);