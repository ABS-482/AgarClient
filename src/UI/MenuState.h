#pragma once

#include <cstddef>
#include <string>
#include <vector>

struct MenuState
{
    int selectedServerIndex = 0;
    int serverListScrollOffset = 0;
    int selectedModeIndex = 0;

    std::vector<size_t> visibleServerIndices;

    std::string selectedMode;

    bool confirmedByClick = false;
};