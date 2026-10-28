
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
            17.0f,
            1.0f,
            1.2f
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
            AssetPath::get("fonts/movavi.otf"),
            16.0f,
            1.0f,
            1.0f
        ),
        menu(
            AssetPath::get("fonts/arial.otf"),
            14.0f,
            1.0f,
            1.0f
        ),
        menuModes(
            AssetPath::get("fonts/movavi.otf"),
            15.0f,
            1.0f,
            1.0f
        ),
        colorPickerTitle(
            AssetPath::get("fonts/movavi.otf"),
            40.0f,
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
    UiFont menu;
    UiFont menuModes;
    UiFont colorPickerTitle;

    UiTextRenderer renderer;
};
