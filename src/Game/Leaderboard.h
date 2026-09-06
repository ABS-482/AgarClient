#pragma once

#include <cstdint>
#include <string>

struct LeaderboardEntry
{
    uint32_t id = 0;
    std::string name;
    uint16_t userLevel = 0;
    uint8_t userType = 0;
    uint32_t urlId = 0;
    uint16_t userLevelSeason = 0;
    bool hasSeason = false;
};