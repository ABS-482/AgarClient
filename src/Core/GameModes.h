#pragma once

#include <string>
#include <vector>

struct GameModeEntry
{
    std::string id;
    std::string name;
    std::string iconPath;
};

inline const std::vector<GameModeEntry> gameModes =
{
    {"FFA", "FFA", "assets/icons/ffa.png"},
    {"FASTFOOD", "FastFood FFA", "assets/icons/fastfoodFfa.png"},
    {"HARDCORE", "HardCore FFA", "assets/icons/hardcoreFfa.png"},
    {"MEGASPLIT", "MegaSplit FFA", "assets/icons/megaSplit.png"},
    {"MEGASPLIT5K", "MegaSplit 5K", "assets/icons/megaSplit5k.png"},
    {"CRAZYSPLIT", "CrazySplit", "assets/icons/crazySplit.png"},
    {"HARDCOREEXP", "HardCore EXP", "assets/icons/hardcoreExp.png"},
    {"ARENA", "ARENA", "assets/icons/arena.png"},
    {"FATBOY-ARENA", "FATBOY ARENA", "assets/icons/fatboyArena.png"},
    {"CRAZY-FFA", "CRAZY FFA", "assets/icons/crazyFfa.png"},
    {"ONESHOT", "OneShot", "assets/icons/oneShot.png"},
    {"BLACKHOLE", "Black Hole", "assets/icons/blackHole.png"},
    {"CLANHOUSE", "ClanHouse", "assets/icons/clanHouse.png"}
};