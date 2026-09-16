#include "MainMenu.h"

#include <algorithm>
#include <cmath>

namespace
{
    constexpr float kServerListHeightScale = 2.0f / 3.0f;
    constexpr float kServerScrollbarWidth = 10.0f;
    constexpr float kServerScrollbarMargin = 6.0f;
    constexpr float kServerScrollbarTrackRadius = 3.0f;
    constexpr float kServerScrollbarThumbRadius = 3.0f;
}

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

MainMenu::MenuLayout MainMenu::computeLayout(
    float screenW,
    float screenH
) const
{
    MenuLayout layout{};

    layout.dialogWidth = 800.0f;
    layout.dialogHeight = 600.0f;

    layout.dialogX = screenW * 0.5f;
    layout.dialogY = screenH * 0.5f;

    layout.dialogLeft =
        layout.dialogX - layout.dialogWidth * 0.5f;

    layout.dialogTop =
        layout.dialogY - layout.dialogHeight * 0.5f;

    layout.modeButtonWidth = 140.0f;
    layout.modeButtonHeight = 35.0f;
    layout.modeButtonGap = 3.0f;

    layout.modeIconSize = 26.0f;
    layout.modeIconGap = 6.0f;

    layout.serverListWidth = 400.0f;
    layout.serverRowHeight = 22.0f;
    layout.serverListGap = 2.0f;
    layout.serverVisibleRows = 20;

    layout.modesLeftX =
        layout.dialogLeft + 155.0f;

    layout.modesTopY =
        layout.dialogTop + 100.0f;

    layout.serversX = layout.dialogLeft + 450.0f;

    layout.serversY =
        layout.modesTopY - 13.0f;

    layout.serverPaddingX = 12.0f;
    layout.serverPaddingY = 15.0f;

    layout.serverTextX =
        layout.serversX -
        layout.serverListWidth * 0.5f +
        layout.serverPaddingX;

    layout.serverButtonWidth = 350.0f;
    layout.serverButtonHeight = 82.0f;

    layout.playButtonWidth = 225.0f;
    layout.playButtonHeight = 55.0f;

    layout.spectateButtonWidth = layout.playButtonWidth;
    layout.spectateButtonHeight = layout.playButtonHeight;

    layout.actionButtonGap = 10.0f;

    const float serverListHeight =
        layout.serverVisibleRows *
        layout.serverRowHeight *
        kServerListHeightScale;

    layout.actionButtonsY =
        layout.serversY +
        serverListHeight +
        25.0f;

    return layout;
}

void MainMenu::update(
    const std::vector<ServerListEntry>& serverList
)
{
    if (gameModes.empty())
    {
        menuState.selectedModeIndex = -1;
        menuState.selectedMode.clear();
        menuState.visibleServerIndices.clear();
        menuState.selectedServerIndex = -1;
        serverScrollOffset = 0.0f;
        return;
    }

    if (menuState.selectedModeIndex < 0 ||
        menuState.selectedModeIndex >= static_cast<int>(gameModes.size()))
    {
        menuState.selectedModeIndex = 0;
    }

    const std::string newSelectedMode =
        gameModes[menuState.selectedModeIndex].id;

    const bool modeChanged =
        newSelectedMode != menuState.selectedMode;

    if (modeChanged)
    {
        serverScrollOffset = 0.0f;
    }

    menuState.selectedMode = newSelectedMode;
    menuState.visibleServerIndices.clear();

    for (size_t i = 0; i < serverList.size(); ++i)
    {
        if (serverList[i].mode == menuState.selectedMode)
        {
            menuState.visibleServerIndices.push_back(i);
        }
    }

    if (modeChanged || menuState.visibleServerIndices.empty())
    {
        menuState.selectedServerIndex =
            menuState.visibleServerIndices.empty()
            ? -1
            : static_cast<int>(
                menuState.visibleServerIndices[0]
                );
    }
}

void MainMenu::moveSelectionDown(
    const std::vector<ServerListEntry>& serverList
)
{
    (void)serverList;

    if (menuState.visibleServerIndices.empty())
        return;

    int currentPos = -1;

    for (size_t i = 0;
        i < menuState.visibleServerIndices.size();
        ++i)
    {
        if (
            static_cast<int>(
                menuState.visibleServerIndices[i]
                ) == menuState.selectedServerIndex
            )
        {
            currentPos = static_cast<int>(i);
            break;
        }
    }

    const int nextPos = std::min(
        currentPos + 1,
        static_cast<int>(
            menuState.visibleServerIndices.size()
            ) - 1
    );

    menuState.selectedServerIndex =
        static_cast<int>(
            menuState.visibleServerIndices[nextPos]
            );
}

void MainMenu::moveSelectionUp()
{
    if (menuState.visibleServerIndices.empty())
        return;

    int currentPos = -1;

    for (size_t i = 0;
        i < menuState.visibleServerIndices.size();
        ++i)
    {
        if (
            static_cast<int>(
                menuState.visibleServerIndices[i]
                ) == menuState.selectedServerIndex
            )
        {
            currentPos = static_cast<int>(i);
            break;
        }
    }

    const int prevPos =
        std::max(currentPos - 1, 0);

    menuState.selectedServerIndex =
        static_cast<int>(
            menuState.visibleServerIndices[prevPos]
            );
}

void MainMenu::handleMouseClick(
    float mouseX,
    float mouseY,
    bool doubleClick,
    const std::vector<ServerListEntry>& serverList,
    float screenW,
    float screenH
)
{
    (void)serverList;

    const MenuLayout layout =
        computeLayout(screenW, screenH);

    // ------------------------------------------------------------
    // Режимы
    // ------------------------------------------------------------

    for (size_t i = 0; i < gameModes.size(); ++i)
    {
        const float y =
            layout.modesTopY +
            static_cast<float>(i) *
            (layout.modeButtonHeight +
                layout.modeButtonGap);

        const float left =
            layout.modesLeftX -
            layout.modeButtonWidth * 0.5f;

        const float right =
            layout.modesLeftX +
            layout.modeButtonWidth * 0.5f;

        const float top =
            y - layout.modeButtonHeight * 0.5f;

        const float bottom =
            y + layout.modeButtonHeight * 0.5f;

        if (
            mouseX >= left &&
            mouseX <= right &&
            mouseY >= top &&
            mouseY <= bottom
            )
        {
            menuState.selectedModeIndex =
                static_cast<int>(i);

            return;
        }
    }

    // ------------------------------------------------------------
// Серверы
// ------------------------------------------------------------

    const float rowStep =
        layout.serverRowHeight +
        layout.serverListGap;

    const float lastModeY =
        layout.modesTopY +
        static_cast<float>(gameModes.size() - 1) *
        (
            layout.modeButtonHeight +
            layout.modeButtonGap
            );

    const float spectateY =
        lastModeY;

    const float playY =
        spectateY -
        layout.playButtonHeight -
        10.0f;

    const float serverListTop =
        layout.serversY;

    const float serverListBottom =
        playY -
        layout.playButtonHeight * 0.5f -
        15.0f;

    const float serverListHeight =
        serverListBottom -
        serverListTop;

    const int firstRow =
        static_cast<int>(
            std::floor(serverScrollOffset)
        );

    const int visibleRowCount =
        std::max(
            1,
            static_cast<int>(
                std::floor(
                    (
                        serverListHeight -
                        layout.serverPaddingY -
                        layout.serverRowHeight * 0.5f
                        ) / rowStep
                )
                ) + 1
        );

    if (menuState.visibleServerIndices.empty())
        return;

    const float maxScroll =
        std::max(
            0.0f,
            (
                layout.serverPaddingY +
                static_cast<float>(
                    menuState.visibleServerIndices.size() - 1
                    ) *
                rowStep +
                layout.serverRowHeight * 0.5f -
                serverListHeight
                ) /
            rowStep
        );

    const int endRow =
        std::min(
            firstRow + visibleRowCount + 1,
            static_cast<int>(
                menuState.visibleServerIndices.size()
                )
        );

    // ------------------------------------------------------------
    // Скроллбар
    // ------------------------------------------------------------

    const float scrollbarTrackTop = layout.serversY + 4.0f;
    const float scrollbarTrackBottom =
        layout.serversY + serverListHeight - 4.0f;
    const float scrollbarTrackHeight =
        scrollbarTrackBottom - scrollbarTrackTop;

    const float contentHeight =
        layout.serverPaddingY +
        static_cast<float>(menuState.visibleServerIndices.size()) * rowStep;

    const float scrollbarThumbHeight =
        std::max(28.0f,
            scrollbarTrackHeight *
            std::min(1.0f, serverListHeight / contentHeight));

    const float scrollbarTravel =
        std::max(0.0f, scrollbarTrackHeight - scrollbarThumbHeight);

    const float scrollbarTrackLeft =
        layout.serversX +
        layout.serverListWidth * 0.5f +
        kServerScrollbarMargin;

    const float scrollbarTrackRight =
        scrollbarTrackLeft + kServerScrollbarWidth;

    const float scrollbarThumbTop =
        scrollbarTrackTop +
        (maxScroll > 0.0f
            ? (serverScrollOffset / maxScroll) * scrollbarTravel
            : 0.0f);

    if (
        mouseX >= scrollbarTrackLeft &&
        mouseX <= scrollbarTrackRight &&
        mouseY >= scrollbarTrackTop &&
        mouseY <= scrollbarTrackBottom
        )
    {
        if (maxScroll > 0.0f && scrollbarTravel > 0.0f)
        {
            const float scrollbarThumbTop =
                scrollbarTrackTop +
                (serverScrollOffset / maxScroll) *
                scrollbarTravel;

            const float scrollbarThumbBottom =
                scrollbarThumbTop +
                scrollbarThumbHeight;

            if (
                mouseY >= scrollbarThumbTop &&
                mouseY <= scrollbarThumbBottom
                )
            {
                draggingServerScrollbar = true;

                // Запоминаем, где именно внутри thumb был клик.
                scrollbarDragOffset =
                    mouseY - scrollbarThumbTop;
            }
            else
            {
                // Клик по дорожке — сразу перемещаем thumb.
                const float targetY =
                    std::clamp(
                        mouseY -
                        scrollbarThumbHeight * 0.5f,
                        scrollbarTrackTop,
                        scrollbarTrackTop + scrollbarTravel
                    );

                const float t =
                    (targetY - scrollbarTrackTop) /
                    scrollbarTravel;

                serverScrollOffset =
                    std::clamp(
                        t * maxScroll,
                        0.0f,
                        maxScroll
                    );
            }
        }

        return;
    }

    for (int row = firstRow; row < endRow; ++row)
    {
        const float visibleRow =
            static_cast<float>(row) -
            serverScrollOffset;

        const float y =
            layout.serversY +
            layout.serverPaddingY +
            visibleRow * rowStep;

        const float left =
            layout.serversX -
            layout.serverListWidth * 0.5f;

        const float right =
            layout.serversX +
            layout.serverListWidth * 0.5f;

        const float top =
            y - layout.serverRowHeight * 0.5f;

        const float bottom =
            y + layout.serverRowHeight * 0.5f;

        if (
            mouseX >= left &&
            mouseX <= right &&
            mouseY >= top &&
            mouseY <= bottom
            )
        {
            const size_t serverIndex =
                menuState.visibleServerIndices[row];

            const bool wasAlreadySelected =
                menuState.selectedServerIndex ==
                static_cast<int>(serverIndex);

            menuState.selectedServerIndex =
                static_cast<int>(serverIndex);

            if (doubleClick || wasAlreadySelected)
            {
                menuState.confirmedByClick = true;
            }

            return;
        }
    }
}

void MainMenu::handleMouseRelease(
    float mouseX,
    float mouseY,
    float screenW,
    float screenH
)
{
    const MenuLayout layout =
        computeLayout(screenW, screenH);

    const float playX =
        layout.serversX;

    const float playY =
        layout.modesTopY +
        static_cast<float>(gameModes.size() - 1) *
        (
            layout.modeButtonHeight +
            layout.modeButtonGap
            ) -
        layout.playButtonHeight -
        10.0f;

    const float playLeft =
        playX -
        layout.playButtonWidth * 0.5f;

    const float playRight =
        playX +
        layout.playButtonWidth * 0.5f;

    const float playTop =
        playY -
        layout.playButtonHeight * 0.5f;

    const float playBottom =
        playY +
        layout.playButtonHeight * 0.5f;

    if (
        mouseX >= playLeft &&
        mouseX <= playRight &&
        mouseY >= playTop &&
        mouseY <= playBottom
        )
    {
        menuState.confirmedByClick = true;
        return;
    }

    const float spectateX =
        layout.serversX;

    const float spectateY =
        layout.modesTopY +
        static_cast<float>(gameModes.size() - 1) *
        (
            layout.modeButtonHeight +
            layout.modeButtonGap
            );

    const float spectateLeft =
        spectateX -
        layout.spectateButtonWidth * 0.5f;

    const float spectateRight =
        spectateX +
        layout.spectateButtonWidth * 0.5f;

    const float spectateTop =
        spectateY -
        layout.spectateButtonHeight * 0.5f;

    const float spectateBottom =
        spectateY +
        layout.spectateButtonHeight * 0.5f;

    if (
        mouseX >= spectateLeft &&
        mouseX <= spectateRight &&
        mouseY >= spectateTop &&
        mouseY <= spectateBottom
        )
    {
        // Пока ничего не делаем.
        return;
    }
}

void MainMenu::handleMouseDrag(
    float mouseX,
    float mouseY,
    bool leftButton,
    float screenW,
    float screenH
)
{
    if (!leftButton)
    {
        draggingServerScrollbar = false;
        scrollbarDragOffset = 0.0f;
        return;
    }

    if (!draggingServerScrollbar)
        return;

    const MenuLayout layout =
        computeLayout(screenW, screenH);

    if (menuState.visibleServerIndices.empty())
        return;

    const float rowStep =
        layout.serverRowHeight +
        layout.serverListGap;

    const float lastModeY =
        layout.modesTopY +
        static_cast<float>(gameModes.size() - 1) *
        (
            layout.modeButtonHeight +
            layout.modeButtonGap
            );

    const float spectateY =
        lastModeY;

    const float playY =
        spectateY -
        layout.playButtonHeight -
        10.0f;

    const float serverListTop =
        layout.serversY;

    const float serverListBottom =
        playY -
        layout.playButtonHeight * 0.5f -
        15.0f;

    const float serverListHeight =
        serverListBottom -
        serverListTop;

    const float scrollbarTrackTop =
        layout.serversY + 4.0f;

    const float scrollbarTrackBottom =
        serverListBottom - 4.0f;

    const float scrollbarTrackHeight =
        scrollbarTrackBottom -
        scrollbarTrackTop;

    const float contentHeight =
        layout.serverPaddingY +
        static_cast<float>(
            menuState.visibleServerIndices.size()
            ) *
        rowStep;

    const float scrollbarThumbHeight =
        std::max(
            28.0f,
            scrollbarTrackHeight *
            std::min(
                1.0f,
                serverListHeight / contentHeight
            )
        );

    const float scrollbarTravel =
        std::max(
            0.0f,
            scrollbarTrackHeight -
            scrollbarThumbHeight
        );

    if (scrollbarTravel <= 0.0f)
        return;

    const float maxScroll =
        std::max(
            0.0f,
            (
                layout.serverPaddingY +
                static_cast<float>(
                    menuState.visibleServerIndices.size() - 1
                    ) *
                rowStep +
                layout.serverRowHeight * 0.5f -
                serverListHeight
                ) /
            rowStep
        );

    if (maxScroll <= 0.0f)
        return;

    // mouseY -> положение верхней границы thumb
    const float thumbTop =
        std::clamp(
            mouseY -
            scrollbarDragOffset,
            scrollbarTrackTop,
            scrollbarTrackTop + scrollbarTravel
        );

    const float t =
        (thumbTop - scrollbarTrackTop) /
        scrollbarTravel;

    serverScrollOffset =
        std::clamp(
            t * maxScroll,
            0.0f,
            maxScroll
        );
}

void MainMenu::handleMouseWheel(
    float mouseX,
    float mouseY,
    float wheel,
    float screenW,
    float screenH
)
{
    if (wheel == 0.0f)
        return;

    const MenuLayout layout =
        computeLayout(screenW, screenH);

    const float lastModeY =
        layout.modesTopY +
        static_cast<float>(gameModes.size() - 1) *
        (
            layout.modeButtonHeight +
            layout.modeButtonGap
            );

    const float spectateY =
        lastModeY;

    const float playY =
        spectateY -
        layout.playButtonHeight -
        10.0f;

    const float serverListTop =
        layout.serversY;

    const float serverListBottom =
        playY -
        layout.playButtonHeight * 0.5f -
        15.0f;

    const float serverListHeight =
        serverListBottom -
        serverListTop;

    const float left =
        layout.serversX -
        layout.serverListWidth * 0.5f;

    const float right =
        layout.serversX +
        layout.serverListWidth * 0.5f;

    const float top =
        serverListTop;

    const float bottom =
        serverListBottom;

    // Колесо работает только над списком серверов.
    if (
        mouseX < left ||
        mouseX > right ||
        mouseY < top ||
        mouseY > bottom
        )
    {
        return;
    }

    if (menuState.visibleServerIndices.empty())
    {
        serverScrollOffset = 0.0f;
        return;
    }

    const float rowStep =
        layout.serverRowHeight +
        layout.serverListGap;

    // Последний элемент должен упереться нижней
    // границей в нижнюю границу списка.
    const float maxScroll =
        std::max(
            0.0f,
            (
                layout.serverPaddingY +
                static_cast<float>(
                    menuState.visibleServerIndices.size() - 1
                    ) *
                rowStep +
                layout.serverRowHeight * 0.5f -
                serverListHeight
                ) /
            rowStep
        );

    if (wheel > 0.0f)
    {
        serverScrollOffset =
            std::max(
                serverScrollOffset - 1.0f,
                0.0f
            );
    }
    else if (wheel < 0.0f)
    {
        serverScrollOffset =
            std::min(
                serverScrollOffset + 1.0f,
                maxScroll
            );
    }
}

bool MainMenu::hasConfirmedSelection(
    const InputState& input,
    const std::vector<ServerListEntry>& serverList
)
{
    const bool confirmed =
        (input.menuConfirmPressed ||
            menuState.confirmedByClick) &&
        !serverList.empty() &&
        menuState.selectedServerIndex >= 0;

    menuState.confirmedByClick = false;

    return confirmed;
}

std::string MainMenu::selectedServerUrl(
    const std::vector<ServerListEntry>& serverList
) const
{
    if (
        menuState.selectedServerIndex < 0 ||
        static_cast<size_t>(
            menuState.selectedServerIndex
            ) >= serverList.size()
        )
    {
        return {};
    }

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
    GLuint blueButtonTexture,
    GLuint redButtonTexture,
    GLuint yellowButtonTexture,
    float screenW,
    float screenH,
    float mouseX,
    float mouseY
)
{
    const MenuLayout layout =
        computeLayout(screenW, screenH);

    // ------------------------------------------------------------
    // Основная панель
    // ------------------------------------------------------------

    uiPanel.drawRoundedCorners(
        layout.dialogX,
        layout.dialogY,
        layout.dialogWidth,
        layout.dialogHeight,
        15.0f,
        15.0f,
        15.0f,
        15.0f,
        1.0f,
        1.0f,
        1.0f,
        1.0f,
        0.0f,
        0.0f,
        0.0f,
        0.0f,
        0.0f,
        screenW,
        screenH
    );
    

    // ------------------------------------------------------------
    // Кнопки режимов
    // ------------------------------------------------------------

    for (size_t i = 0; i < gameModes.size(); ++i)
    {
        const float x =
            layout.modesLeftX;

        const float y =
            layout.modesTopY +
            static_cast<float>(i) *
            (layout.modeButtonHeight +
                layout.modeButtonGap);

        const bool selected =
            static_cast<int>(i) ==
            menuState.selectedModeIndex;

        const float left =
            x - layout.modeButtonWidth * 0.5f;

        const float right =
            x + layout.modeButtonWidth * 0.5f;

        const float top =
            y - layout.modeButtonHeight * 0.5f;

        const float bottom =
            y + layout.modeButtonHeight * 0.5f;

        const bool hovered =
            mouseX >= left &&
            mouseX <= right &&
            mouseY >= top &&
            mouseY <= bottom;

        float fillR;
        float fillG;
        float fillB;

        float borderR;
        float borderG;
        float borderB;

        if (selected)
        {
            fillR = 0.3608f;
            fillG = 0.7216f;
            fillB = 0.3608f;

            borderR = 0.2980f;
            borderG = 0.6824f;
            borderB = 0.2980f;
        }
        else if (hovered)
        {
            // #3071a9
            fillR = 48.0f / 255.0f;
            fillG = 113.0f / 255.0f;
            fillB = 169.0f / 255.0f;

            // #285e8e
            borderR = 40.0f / 255.0f;
            borderG = 94.0f / 255.0f;
            borderB = 142.0f / 255.0f;
        }
        else
        {
            fillR = 0.2588f;
            fillG = 0.5451f;
            fillB = 0.7922f;

            borderR = 0.2078f;
            borderG = 0.4941f;
            borderB = 0.7412f;
        }

        iconRenderer.draw(
            blueButtonTexture,
            x,
            y,
            layout.modeButtonWidth,
            layout.modeButtonHeight,
            screenW,
            screenH
        );
    }

    // ------------------------------------------------------------
    // Иконки режимов
    // ------------------------------------------------------------

    for (size_t i = 0; i < gameModes.size(); ++i)
    {
        const float x =
            layout.modesLeftX;

        const float y =
            layout.modesTopY +
            static_cast<float>(i) *
            (layout.modeButtonHeight +
                layout.modeButtonGap);

        const GLuint iconTexture =
            gameModeIconTextures[i];

        if (iconTexture == 0)
            continue;

        const float buttonLeftEdge =
            x - layout.modeButtonWidth * 0.5f;

        const float iconCenterX =
            buttonLeftEdge -
            layout.modeIconGap -
            layout.modeIconSize * 0.5f;

        iconRenderer.draw(
            iconTexture,
            iconCenterX,
            y,
            layout.modeIconSize,
            layout.modeIconSize,
            screenW,
            screenH
        );
    }

    // ------------------------------------------------------------
// Список серверов
// ------------------------------------------------------------

    const float lastModeY =
        layout.modesTopY +
        static_cast<float>(gameModes.size() - 1) *
        (
            layout.modeButtonHeight +
            layout.modeButtonGap
            );

    const float spectateY =
        lastModeY;

    const float playY =
        spectateY -
        layout.playButtonHeight -
        10.0f;

    // Верхняя граница списка
    const float serverListTop =
        layout.serversY;

    // Нижняя граница списка — до верхнего края Play
    const float serverListBottom =
        playY -
        layout.playButtonHeight * 0.5f -
        15.0f;

    const float serverListHeight =
        serverListBottom -
        serverListTop;

    textRenderer.begin();

    glEnable(GL_SCISSOR_TEST);

    glScissor(
        static_cast<GLint>(
            layout.serversX -
            layout.serverListWidth * 0.5f
            ),
        static_cast<GLint>(
            screenH -
            (layout.serversY + serverListHeight)
            ),
        static_cast<GLint>(
            layout.serverListWidth
            ),
        static_cast<GLint>(
            serverListHeight
            )
    );

    const float rowStep =
        layout.serverRowHeight +
        layout.serverListGap;

    const int firstRow =
        static_cast<int>(
            std::floor(serverScrollOffset)
            );

    const int visibleRowCount =
        std::max(
            1,
            static_cast<int>(
                std::floor(
                    (
                        serverListHeight -
                        layout.serverPaddingY -
                        layout.serverRowHeight * 0.5f
                        ) / rowStep
                )
                ) + 1
        );

    const int endRow =
        std::min(
            firstRow +
            visibleRowCount +
            1,
            static_cast<int>(
                menuState.visibleServerIndices.size()
                )
        );

    for (int row = firstRow; row < endRow; ++row)
    {
        const size_t serverIndex =
            menuState.visibleServerIndices[row];

        const auto& server =
            serverList[serverIndex];

        const float visibleRow =
            static_cast<float>(row) -
            serverScrollOffset;

        const float y =
            layout.serversY +
            layout.serverPaddingY +
            visibleRow * rowStep;

        const bool selected =
            static_cast<int>(serverIndex) ==
            menuState.selectedServerIndex;

        const float left =
            layout.serversX -
            layout.serverListWidth * 0.5f;

        const float right =
            layout.serversX +
            layout.serverListWidth * 0.5f;

        const float top =
            y - layout.serverRowHeight * 0.5f;

        const float bottom =
            y + layout.serverRowHeight * 0.5f;

        const bool hovered =
            mouseX >= left &&
            mouseX <= right &&
            mouseY >= top &&
            mouseY <= bottom;

        if (selected)
        {
            // #4691CD
            uiPanel.draw(
                layout.serversX,
                y,
                layout.serverListWidth - 6.0f,
                layout.serverRowHeight - 2.0f,
                3.0f,
                70.0f / 255.0f,
                145.0f / 255.0f,
                205.0f / 255.0f,
                1.0f,
                0.0f,
                0.0f,
                0.0f,
                0.0f,
                0.0f,
                screenW,
                screenH
            );
        }
        else if (hovered)
        {
            // #6FA9D1
            uiPanel.draw(
                layout.serversX,
                y,
                layout.serverListWidth - 6.0f,
                layout.serverRowHeight - 2.0f,
                3.0f,
                111.0f / 255.0f,
                169.0f / 255.0f,
                209.0f / 255.0f,
                1.0f,
                0.0f,
                0.0f,
                0.0f,
                0.0f,
                0.0f,
                screenW,
                screenH
            );
        }

        const std::string& line =
            server.displayText;

        textRenderer.addTextLeftAligned(
            gmFont,
            line,
            layout.serverTextX,
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

    glDisable(GL_SCISSOR_TEST);

    const float playX = layout.serversX;
    const float spectateX = layout.serversX;

    iconRenderer.draw(
        redButtonTexture,
        playX,
        playY,
        layout.playButtonWidth,
        layout.playButtonHeight,
        screenW,
        screenH
    );

    iconRenderer.draw(
        yellowButtonTexture,
        spectateX,
        spectateY,
        layout.spectateButtonWidth,
        layout.spectateButtonHeight,
        screenW,
        screenH
    );

    textRenderer.begin();

    textRenderer.addText(
        gmFont,
        "Play",
        playX,
        playY,
        0.24f
    );

    textRenderer.addText(
        gmFont,
        "Spectate",
        spectateX,
        spectateY,
        0.24f
    );

    textRenderer.end(
        gmFont,
        screenW,
        screenH,
        1.0f,
        1.0f,
        1.0f
    );

    // ------------------------------------------------------------
    // Скроллбар списка серверов
    // ------------------------------------------------------------

    const float maxScroll =
        std::max(
            0.0f,
            (
                layout.serverPaddingY +
                static_cast<float>(
                    menuState.visibleServerIndices.size() - 1
                    ) *
                rowStep +
                layout.serverRowHeight * 0.5f -
                serverListHeight
                ) /
            rowStep
        );

    if (maxScroll > 0.0f)
    {
        const float scrollbarTrackTop =
            layout.serversY + 4.0f;

        const float scrollbarTrackBottom =
            layout.serversY +
            serverListHeight -
            4.0f;

        const float contentHeight =
            layout.serverPaddingY +
            static_cast<float>(
                menuState.visibleServerIndices.size()
                ) *
            rowStep;

        const float scrollbarTrackHeight =
            scrollbarTrackBottom -
            scrollbarTrackTop;

        const float scrollbarThumbHeight =
            std::max(
                28.0f,
                scrollbarTrackHeight *
                std::min(
                    1.0f,
                    serverListHeight / contentHeight
                )
            );

        const float scrollbarTravel =
            std::max(
                0.0f,
                scrollbarTrackHeight -
                scrollbarThumbHeight
            );

        const float scrollbarX =
            layout.serversX +
            layout.serverListWidth * 0.5f +
            kServerScrollbarMargin +
            kServerScrollbarWidth * 0.5f;

        const float scrollbarTrackLeft =
            layout.serversX +
            layout.serverListWidth * 0.5f +
            kServerScrollbarMargin;

        const float scrollbarTrackRight =
            scrollbarTrackLeft +
            kServerScrollbarWidth;

        const float scrollbarThumbTop =
            scrollbarTrackTop +
            (serverScrollOffset / maxScroll) *
            scrollbarTravel;

        const bool scrollbarHovered =
            mouseX >= scrollbarTrackLeft &&
            mouseX <= scrollbarTrackRight &&
            mouseY >= scrollbarThumbTop &&
            mouseY <=
            scrollbarThumbTop +
            scrollbarThumbHeight;

        uiPanel.draw(
            scrollbarX,
            (scrollbarTrackTop + scrollbarTrackBottom) * 0.5f,
            kServerScrollbarWidth,
            scrollbarTrackHeight,
            kServerScrollbarTrackRadius,
            230.0f / 255.0f,
            230.0f / 255.0f,
            230.0f / 255.0f,
            1.0f,
            0.0f, 0.0f, 0.0f, 0.0f,
            0.0f,
            screenW,
            screenH
        );

        uiPanel.draw(
            scrollbarX,
            scrollbarThumbTop +
            scrollbarThumbHeight * 0.5f,
            kServerScrollbarWidth,
            scrollbarThumbHeight,
            kServerScrollbarThumbRadius,
            scrollbarHovered
            ? 120.0f / 255.0f
            : 150.0f / 255.0f,
            scrollbarHovered
            ? 120.0f / 255.0f
            : 150.0f / 255.0f,
            scrollbarHovered
            ? 120.0f / 255.0f
            : 150.0f / 255.0f,
            1.0f,
            0.0f, 0.0f, 0.0f, 0.0f,
            0.0f,
            screenW,
            screenH
        );
    }

    // ------------------------------------------------------------
    // Названия режимов
    // ------------------------------------------------------------

    textRenderer.begin();

    for (size_t i = 0; i < gameModes.size(); ++i)
    {
        const float x =
            layout.modesLeftX;

        const float y =
            layout.modesTopY +
            static_cast<float>(i) *
            (layout.modeButtonHeight +
                layout.modeButtonGap);

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