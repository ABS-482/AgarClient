#pragma once

#include <glad/glad.h>

#include <vector>
#include <string>

#include "../Input/InputState.h"
#include "../Core/GameModes.h"
#include "../Graphics/Font.h"
#include "../Graphics/IconRenderer.h"
#include "../Graphics/TextRenderer.h"
#include "../Graphics/UIPanel.h"
#include "../Network/ServerListFetcher.h"
#include "MenuState.h"

class MainMenu
{
public:
    MainMenu(
        UIPanel& uiPanel,
        TextRenderer& textRenderer,
        IconRenderer& iconRenderer,
        Font& gmFont,
        Font& menuFont
    );

    void draw(
        const std::vector<ServerListEntry>& serverList,
        const std::vector<GLuint>& gameModeIconTextures,
        float screenW,
        float screenH
    );

    void update(
        const std::vector<ServerListEntry>& serverList
    );

    void moveSelectionDown(const std::vector<ServerListEntry>& serverList);
    void moveSelectionUp();

    bool hasConfirmedSelection(
        const InputState& input,
        const std::vector<ServerListEntry>& serverList
    );

    std::string selectedServerUrl(
        const std::vector<ServerListEntry>& serverList
    ) const;

    MenuState& state();

private:
    UIPanel& uiPanel;
    TextRenderer& textRenderer;
    IconRenderer& iconRenderer;
    Font& gmFont;
    Font& menuFont;

    MenuState menuState;
};