
#pragma once

#include "UiFont.h"
#include "UiTextRenderer.h"
#include "../Core/AssetPath.h"

class UiFonts
{
public:
    UiFonts()
        : leaderboard(
            AssetPath::get("fonts/arial.otf"),
            20.0f,
            1.0f,
            1.0f
        ),
        chat(
            AssetPath::get("fonts/arial.otf"),
            13.0f,
            1.0f,
            1.0f
        ),
        serverInfo(
            AssetPath::get("fonts/movavi.otf"),
            12.0f,
            1.0f,
            1.0f
        ),
        performance(
            AssetPath::get("fonts/movavi.otf"),
            13.0f,
            1.0f,
            1.0f
        ),
        playerStats(
            AssetPath::get("fonts/arial.otf"),
            20.0f,
            1.0f,
            1.0f
        )
    {
    }

    UiFont leaderboard;
    UiFont chat;
    UiFont serverInfo;
    UiFont performance;
    UiFont playerStats;

    UiTextRenderer renderer;
};
