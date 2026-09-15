#include "MainMenu.h"

MainMenu::MainMenu(
    UIPanel& uiPanel,
    TextRenderer& textRenderer,
    IconRenderer& iconRenderer,
    Font& gmFont,
    Font& menuFont
)
    : uiPanel(uiPanel),
    textRenderer(textRenderer),
    iconRenderer(iconRenderer),
    gmFont(gmFont),
    menuFont(menuFont)
{
}

void MainMenu::update(
    const std::vector<ServerListEntry>& serverList
)
{
    menuState.visibleServerIndices.clear();

    menuState.selectedMode =
        gameModes[menuState.selectedModeIndex].id;

    for (size_t i = 0; i < serverList.size(); ++i)
    {
        if (serverList[i].mode == menuState.selectedMode)
        {
            menuState.visibleServerIndices.push_back(i);
        }
    }
}

void MainMenu::moveSelectionDown(
    const std::vector<ServerListEntry>& serverList
)
{
    if (serverList.empty())
        return;

    menuState.selectedServerIndex = std::min(
        menuState.selectedServerIndex + 1,
        static_cast<int>(serverList.size()) - 1
    );
}

void MainMenu::moveSelectionUp()
{
    menuState.selectedServerIndex =
        std::max(menuState.selectedServerIndex - 1, 0);
}

bool MainMenu::hasConfirmedSelection(
    const InputState& input,
    const std::vector<ServerListEntry>& serverList
)
{
    return input.menuConfirmPressed && !serverList.empty();
}

std::string MainMenu::selectedServerUrl(
    const std::vector<ServerListEntry>& serverList
) const
{
    const auto& chosen =
        serverList[menuState.selectedServerIndex];

    const size_t colonPos =
        chosen.address.find(':');

    if (colonPos == std::string::npos)
        return {};

    const std::string host =
        chosen.address.substr(0, colonPos);

    return "wss://" + host + ":443";
}

MenuState& MainMenu::state()
{
    return menuState;
}

void MainMenu::draw(
    const std::vector<ServerListEntry>& serverList,
    const std::vector<GLuint>& gameModeIconTextures,
    float screenW,
    float screenH
)
{
    constexpr float dialogWidth = 500.0f;
    constexpr float dialogHeight = 600.0f;

    const float dialogX = screenW * 0.5f;
    const float dialogY = screenH * 0.5f;

    const float dialogLeft =
        dialogX - dialogWidth * 0.5f;

    const float dialogTop =
        dialogY - dialogHeight * 0.5f;

    constexpr float modeButtonWidth = 130.0f;
    constexpr float modeButtonHeight = 28.0f;
    constexpr float modeButtonGap = 3.0f;

    constexpr float modeIconSize = 26.0f;
    constexpr float modeIconGap = 6.0f;

    constexpr float serverListWidth = 275.0f;
    constexpr float serverRowHeight = 22.0f;
    constexpr float serverListGap = 2.0f;
    constexpr int serverVisibleRows = 20;

    const float modesLeftX =
        dialogLeft + 110.0f;

    const float modesTopY =
        dialogTop + 185.0f;

    const float serversX =
        dialogLeft + 350.0f;

    const float serversY =
        modesTopY;

    constexpr float serverPaddingX = 12.0f;
    constexpr float serverPaddingY = 6.0f;

    const float serverTextX =
        serversX - serverListWidth * 0.5f + serverPaddingX;

    uiPanel.drawRoundedCorners(
        dialogX,
        dialogY,
        dialogWidth,
        dialogHeight,

        250.0f,
        250.0f,
        50.0f,
        50.0f,

        1.0f, 1.0f, 1.0f, 1.0f,
        0.0f, 0.0f, 0.0f, 0.0f,
        0.0f,

        screenW,
        screenH
    );

    textRenderer.begin();

    textRenderer.addText(
        menuFont,
        "PetriDish",
        screenW * 0.5f,
        dialogY - dialogHeight * 0.5f + 65.0f,
        0.55f
    );

    textRenderer.addText(
        menuFont,
        "Total players online: 1134",
        screenW * 0.5f,
        dialogY - dialogHeight * 0.5f + 105.0f,
        0.22f
    );

    textRenderer.end(
        menuFont,
        screenW,
        screenH,
        66.0f / 255.0f,
        139.0f / 255.0f,
        202.0f / 255.0f
    );

    // Mode buttons
    for (size_t i = 0; i < gameModes.size(); ++i)
    {
        const float x = modesLeftX;

        const float y =
            modesTopY +
            static_cast<float>(i) *
            (modeButtonHeight + modeButtonGap);

        const bool selected =
            static_cast<int>(i) == menuState.selectedModeIndex;

        uiPanel.drawRoundedCorners(
            x, y,
            modeButtonWidth, modeButtonHeight,
            5.0f, 5.0f, 5.0f, 5.0f,

            selected ? 0.3608f : 0.2588f,
            selected ? 0.7216f : 0.5451f,
            selected ? 0.3608f : 0.7922f,
            1.0f,

            selected ? 0.2980f : 0.2078f,
            selected ? 0.6824f : 0.4941f,
            selected ? 0.2980f : 0.7412f,
            1.0f,

            1.0f,
            screenW,
            screenH
        );
    }

    // Mode icons
    for (size_t i = 0; i < gameModes.size(); ++i)
    {
        const float x = modesLeftX;

        const float y =
            modesTopY +
            static_cast<float>(i) *
            (modeButtonHeight + modeButtonGap);

        const GLuint iconTexture =
            gameModeIconTextures[i];

        if (iconTexture == 0)
            continue;

        const float buttonLeftEdge =
            x - modeButtonWidth * 0.5f;

        const float iconCenterX =
            buttonLeftEdge
            - modeIconGap
            - modeIconSize * 0.5f;

        iconRenderer.draw(
            iconTexture,
            iconCenterX,
            y,
            modeIconSize,
            modeIconSize,
            screenW,
            screenH
        );
    }

    // Server list
    textRenderer.begin();

    const float serverListHeight =
        serverVisibleRows * serverRowHeight;

    uiPanel.draw(
        serversX,
        serversY + serverListHeight * 0.5f,
        serverListWidth,
        serverListHeight,
        4.0f,

        1.0f,
        1.0f,
        1.0f,
        1.0f,

        204.0f / 255.0f,
        204.0f / 255.0f,
        204.0f / 255.0f,
        1.0f,

        1.0f,

        screenW,
        screenH
    );

    for (size_t row = 0;
        row < menuState.visibleServerIndices.size();
        ++row)
    {
        const size_t serverIndex =
            menuState.visibleServerIndices[row];

        const auto& server =
            serverList[serverIndex];

        const float y =
            serversY +
            serverPaddingY +
            static_cast<float>(row) *
            (serverRowHeight + serverListGap);

        const bool selected =
            static_cast<int>(serverIndex) ==
            menuState.selectedServerIndex;

        (void)selected;

        const std::string line =
            server.sname +
            "   " +
            std::to_string(server.online) +
            "/" +
            std::to_string(server.connectlimit);

        textRenderer.addTextLeftAligned(
            gmFont,
            line,
            serverTextX,
            y,
            0.16f
        );
    }

    textRenderer.end(
        gmFont,
        screenW,
        screenH,
        0.0f,
        0.0f,
        0.0f
    );

    // Mode names
    textRenderer.begin();

    for (size_t i = 0; i < gameModes.size(); ++i)
    {
        const float x = modesLeftX;

        const float y =
            modesTopY +
            static_cast<float>(i) *
            (modeButtonHeight + modeButtonGap);

        textRenderer.addText(
            gmFont,
            gameModes[i].name,
            x,
            y,
            0.18f
        );
    }

    textRenderer.end(
        gmFont,
        screenW,
        screenH,
        1.0f,
        1.0f,
        1.0f
    );
}