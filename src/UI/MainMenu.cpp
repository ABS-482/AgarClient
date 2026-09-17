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
    layout.serverRowHeight = 34.0f;
    layout.serverListGap = 4.0f;
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

    layout.playButtonWidth = 113.0f;
    layout.playButtonHeight = 40.0f;

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
                menuState.joinMode =
                    MenuState::JoinMode::Play;

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
        menuState.joinMode =
            MenuState::JoinMode::Play;

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
        menuState.joinMode =
            MenuState::JoinMode::Spectate;

        menuState.confirmedByClick = true;
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
        20.0f,
        20.0f,
        20.0f,
        20.0f,
        29.0f / 255.0f,
        28.0f / 255.0f,
        33.0f / 255.0f,
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
            (
                layout.modeButtonHeight +
                layout.modeButtonGap
                );

        const bool selected =
            static_cast<int>(i) ==
            menuState.selectedModeIndex;

        // Левая и правая границы самой кнопки
        const float buttonLeft =
            x -
            layout.modeButtonWidth * 0.5f;

        const float buttonRight =
            x +
            layout.modeButtonWidth * 0.5f;

        // Позиция иконки
        const float iconCenterX =
            buttonLeft -
            layout.modeIconGap -
            layout.modeIconSize * 0.5f;

        // Границы всего элемента: иконка + кнопка
        const float hoverPadding = 8.0f;

        const float hoverLeft =
            iconCenterX -
            layout.modeIconSize * 0.5f -
            hoverPadding;

        const float hoverRight =
            buttonRight;

        const float top =
            y -
            layout.modeButtonHeight * 0.5f;

        const float bottom =
            y +
            layout.modeButtonHeight * 0.5f;

        const bool hovered =
            mouseX >= hoverLeft &&
            mouseX <= hoverRight &&
            mouseY >= top &&
            mouseY <= bottom;

        // --------------------------------------------------------
        // Selected / Hover
        // --------------------------------------------------------

        if (selected)
        {
            const float selectedWidth =
                hoverRight - hoverLeft;

            const float selectedCenterX =
                (hoverLeft + hoverRight) * 0.5f;

            uiPanel.draw(
                selectedCenterX,
                y,
                selectedWidth,
                layout.modeButtonHeight,
                18.0f,

                // background — rgba(255,108,0,0.20)
                1.0f,
                108.0f / 255.0f,
                0.0f,
                0.20f,

                // border — отсутствует
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
            const float hoverWidth =
                hoverRight - hoverLeft;

            const float hoverCenterX =
                (hoverLeft + hoverRight) * 0.5f;

            uiPanel.draw(
                hoverCenterX,
                y,
                hoverWidth,
                layout.modeButtonHeight,
                18.0f,

                // background — прозрачный
                0.0f,
                0.0f,
                0.0f,
                0.0f,

                // border — rgba(255,255,255,0.15)
                1.0f,
                1.0f,
                1.0f,
                0.15f,

                // border thickness
                1.0f,

                screenW,
                screenH
            );
        }

        // --------------------------------------------------------
        // Название режима
        // --------------------------------------------------------

        textRenderer.addTextLeftAligned(
            gmFont,
            gameModes[i].name,
            buttonLeft + 10.0f,
            y,
            0.16f
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

    // Нижняя граница списка — с отступом 15 px от Play
    const float serverListBottom =
        playY -
        layout.playButtonHeight * 0.5f -
        15.0f;

    const float serverListHeight =
        serverListBottom -
        serverListTop;

    // ------------------------------------------------------------
    // Тёмная панель списка
    // ------------------------------------------------------------

    uiPanel.draw(
        layout.serversX,
        serverListTop +
        serverListHeight * 0.5f,
        layout.serverListWidth,
        serverListHeight,
        12.0f,

        // background
        0.0f,
        0.0f,
        0.0f,
        0.0f,

        // border
        0.0f,
        0.0f,
        0.0f,
        0.0f,

        1.0f,

        screenW,
        screenH
    );

    textRenderer.begin();

    glEnable(GL_SCISSOR_TEST);

    glScissor(
        static_cast<GLint>(
            layout.serversX -
            layout.serverListWidth * 0.5f
            ),
        static_cast<GLint>(
            screenH -
            (serverListTop + serverListHeight)
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
            y -
            layout.serverRowHeight * 0.5f;

        const float bottom =
            y +
            layout.serverRowHeight * 0.5f;

        const bool hovered =
            mouseX >= left &&
            mouseX <= right &&
            mouseY >= top &&
            mouseY <= bottom;

        // --------------------------------------------------------
        // Selected / Hover
        // --------------------------------------------------------

        if (selected)
        {
            uiPanel.draw(
                layout.serversX,
                y,
                layout.serverListWidth - 6.0f,
                layout.serverRowHeight,
                18.0f,

                38.0f / 255.0f,
                111.0f / 255.0f,
                255.0f / 255.0f,
                0.75f,

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
            uiPanel.draw(
                layout.serversX,
                y,
                layout.serverListWidth - 6.0f,
                layout.serverRowHeight - 2.0f,
                18.0f,

                // background — прозрачный
                0.0f,
                0.0f,
                0.0f,
                0.0f,

                // border — rgba(255,255,255,0.15)
                1.0f,
                1.0f,
                1.0f,
                0.15f,

                // border thickness
                1.0f,

                screenW,
                screenH
            );
        }

        // --------------------------------------------------------
        // Server text
        // --------------------------------------------------------

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
        198.0f / 255.0f,
        198.0f / 255.0f,
        198.0f / 255.0f
    );

    glDisable(GL_SCISSOR_TEST);

    // ------------------------------------------------------------
    // Play / Spectate
    // ------------------------------------------------------------

    const float playX =
        layout.serversX;

    const float spectateX =
        layout.serversX;

    // ------------------------------------------------------------
    // Play
    // ------------------------------------------------------------

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

    const bool playHovered =
        mouseX >= playLeft &&
        mouseX <= playRight &&
        mouseY >= playTop &&
        mouseY <= playBottom;

    uiPanel.draw(
        playX,
        playY,
        layout.playButtonWidth,
        layout.playButtonHeight,
        20.0f,

        // background
        playHovered
        ? 30.0f / 255.0f
        : 38.0f / 255.0f,
        playHovered
        ? 88.0f / 255.0f
        : 111.0f / 255.0f,
        playHovered
        ? 204.0f / 255.0f
        : 255.0f / 255.0f,
        1.0f,

        // border — none
        0.0f,
        0.0f,
        0.0f,
        0.0f,

        0.0f,

        screenW,
        screenH
    );

    // ------------------------------------------------------------
    // Spectate — пока оставляем старый вид
    // ------------------------------------------------------------

    uiPanel.draw(
        spectateX,
        spectateY,
        layout.spectateButtonWidth,
        layout.spectateButtonHeight,
        25.0f,

        // background — none
        0.0f,
        0.0f,
        0.0f,
        0.0f,

        // border — #266fff
        38.0f / 255.0f,
        111.0f / 255.0f,
        255.0f / 255.0f,
        1.0f,

        // border thickness
        1.0f,

        screenW,
        screenH
    );

    // ------------------------------------------------------------
    // Текст Play
    // ------------------------------------------------------------

    textRenderer.begin();

    textRenderer.addText(
        menuFont,
        "Play",
        playX,
        playY,
        0.24f
    );

    textRenderer.end(
        menuFont,
        screenW,
        screenH,
        1.0f,
        1.0f,
        1.0f
    );

    // ------------------------------------------------------------
    // Текст Spectate
    // ------------------------------------------------------------

    textRenderer.begin();

    textRenderer.addText(
        menuFont,
        "Spectate",
        spectateX,
        spectateY,
        0.24f
    );

    textRenderer.end(
        menuFont,
        screenW,
        screenH,
        38.0f / 255.0f,
        111.0f / 255.0f,
        255.0f / 255.0f
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
            0.0f,
            0.0f,
            0.0f,
            0.05f,
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

            // thumb
            1.0f,
            1.0f,
            1.0f,
            0.10f,

            0.0f,
            0.0f,
            0.0f,
            0.0f,

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
            0.16f
        );
    }

    textRenderer.end(
        gmFont,
        screenW,
        screenH,
        0.776f, 0.776f, 0.776f
    );
}