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
    struct MenuLayout
    {
        float dialogX, dialogY, dialogWidth, dialogHeight, dialogLeft, dialogTop;

        float modesLeftX, modesTopY;
        float modeButtonWidth, modeButtonHeight, modeButtonGap;

        float modeIconSize, modeIconGap;

        float serversX, serversY;
        float serverListWidth, serverRowHeight, serverListGap;
        float serverButtonWidth, serverButtonHeight;
        float serverPaddingX, serverPaddingY;

        float playButtonWidth, playButtonHeight;
        float spectateButtonWidth, spectateButtonHeight;
        float actionButtonGap;
        float actionButtonsY;

        int serverVisibleRows;
        float serverTextX;
    };

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
        GLuint blueButtonTexture,
        GLuint redButtonTexture,
        GLuint yellowButtonTexture,
        float screenW,
        float screenH,
        float mouseX,
        float mouseY
    );

    void update(
        const std::vector<ServerListEntry>& serverList
    );

    void moveSelectionDown(const std::vector<ServerListEntry>& serverList);
    void moveSelectionUp();

    void handleMouseClick(
        float mouseX, float mouseY,
        bool doubleClick,
        const std::vector<ServerListEntry>& serverList,
        float screenW, float screenH
    );

    void handleMouseRelease(
        float mouseX,
        float mouseY,
        float screenW,
        float screenH
    );

    void handleMouseWheel(
        float mouseX,
        float mouseY,
        float wheel,
        float screenW,
        float screenH
    );

    void handleMouseDrag(
        float mouseX,
        float mouseY,
        bool leftButton,
        float screenW,
        float screenH
    );

    bool hasConfirmedSelection(
        const InputState& input,
        const std::vector<ServerListEntry>& serverList
    );

    std::string selectedServerUrl(
        const std::vector<ServerListEntry>& serverList
    ) const;

    MenuState& state();

private:
    MenuLayout computeLayout(float screenW, float screenH) const;

    UIPanel& uiPanel;
    TextRenderer& textRenderer;
    IconRenderer& iconRenderer;
    Font& gmFont;
    Font& menuFont;

    MenuState menuState;
    bool draggingServerScrollbar = false;
    float scrollbarDragOffset = 0.0f;
    float serverScrollOffset = 0.0f;

};