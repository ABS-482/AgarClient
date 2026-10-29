#include "MainMenu.h"
#include "../Graphics/Shader.h"
#include "../Graphics/SkinMesh.h"
#include "../Graphics/SkinManager.h"
#include "../Network/UserSkinDatabase.h"
#include "../Core/AssetPath.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <string_view>

namespace
{
    constexpr float kServerListHeightScale = 2.0f / 3.0f;
    constexpr float kServerScrollbarWidth = 10.0f;
    constexpr float kServerScrollbarMargin = 6.0f;
    constexpr float kServerScrollbarTrackRadius = 3.0f;
    constexpr float kServerScrollbarThumbRadius = 3.0f;

    // displayText запечён один раз при фетче списка (см. ServerListFetcher)
    // и содержит "<online>/<connectlimit>" где-то внутри своего текста.
    // RealtimeInfoClient обновляет server.online в реальном времени, но
    // не трогает саму строку — поэтому здесь на лету подменяем только
    // число online прямо перед известным "/connectlimit", не трогая
    // остальное форматирование (имя, режим, карту и т.д.), которое
    // мы не парсим и формат которого нам не известен.
    std::string liveServerDisplayText(const ServerListEntry& server)
    {
        const std::string anchor =
            "/" + std::to_string(server.connectlimit);

        const size_t slashPos =
            server.displayText.find(anchor);

        if (slashPos == std::string::npos)
            return server.displayText; // неожиданный формат — не трогаем

        size_t numStart = slashPos;

        while (
            numStart > 0 &&
            std::isdigit(
                static_cast<unsigned char>(
                    server.displayText[numStart - 1]
                    )
            )
            )
        {
            --numStart;
        }

        if (numStart == slashPos)
            return server.displayText; // перед "/" цифр не нашли

        std::string result = server.displayText;

        result.replace(
            numStart,
            slashPos - numStart,
            std::to_string(server.online)
        );

        return result;
    }

    bool isUtf8Continuation(unsigned char c)
    {
        return (c & 0xC0) == 0x80;
    }

    std::size_t utf8CharacterCount(std::string_view text)
    {
        std::size_t count = 0;

        for (unsigned char c : text)
        {
            if (!isUtf8Continuation(c))
                ++count;
        }

        return count;
    }

    std::string utf8Prefix(
        std::string_view text,
        std::size_t maxCharacters
    )
    {
        std::string result;
        std::size_t i = 0;
        std::size_t count = 0;

        while (i < text.size() && count < maxCharacters)
        {
            std::size_t next = i + 1;

            while (
                next < text.size() &&
                isUtf8Continuation(
                    static_cast<unsigned char>(text[next])
                ))
            {
                ++next;
            }

            result.append(text.substr(i, next - i));

            i = next;
            ++count;
        }

        return result;
    }

    std::size_t utf8ByteOffset(
        std::string_view text,
        std::size_t characterIndex
    )
    {
        std::size_t offset = 0;

        while (offset < text.size() && characterIndex > 0)
        {
            ++offset;

            while (
                offset < text.size() &&
                isUtf8Continuation(
                    static_cast<unsigned char>(text[offset])
                ))
            {
                ++offset;
            }

            --characterIndex;
        }

        return offset;
    }

    std::string utf8WithCursor(
        std::string_view text,
        std::size_t characterIndex
    )
    {
        std::string result(text);

        const std::size_t characterCount =
            utf8CharacterCount(result);

        const std::size_t byteOffset =
            utf8ByteOffset(
                result,
                std::min(characterIndex, characterCount)
            );

        result.insert(byteOffset, "|");

        return result;
    }

}

MainMenu::MainMenu(
    UIPanel& uiPanel,
    UiFonts& uiFonts,
    IconRenderer& iconRenderer,
    Shader& skinShader,
    SkinMesh& skinMesh,
    SkinManager& skinManager,
    UserSkinDatabase& userSkinDatabase
)
    : uiPanel(uiPanel),
    uiFonts(uiFonts),
    iconRenderer(iconRenderer),
    skinShader(skinShader),
    skinMesh(skinMesh),
    skinManager(skinManager),
    userSkinDatabase(userSkinDatabase),
    m_colorPicker(
        iconRenderer,
        uiPanel,
        uiFonts.renderer,
        uiFonts.colorPickerTitle
    )
{
    m_uSkinCenter =
        skinShader.uniformLocation("uCenter");

    m_uSkinBaseColor =
        skinShader.uniformLocation("uBaseColor");

    m_uSkinDepth =
        skinShader.uniformLocation("uDepth");

    m_uSkinRadius =
        skinShader.uniformLocation("uRadius");

    m_uSkinCameraPos =
        skinShader.uniformLocation("uCameraPos");

    m_uSkinZoom =
        skinShader.uniformLocation("uZoom");

    m_uSkinScreenSize =
        skinShader.uniformLocation("uScreenSize");

    m_uSkinTexture =
        skinShader.uniformLocation("uSkin");

    m_uSkinViewportSize =
        skinShader.uniformLocation("uViewportSize");

    const std::string circledBlobPath =
        AssetPath::get("ui/circledBlob.png");

    m_circledBlobTexture =
        iconRenderer.loadTexture(circledBlobPath);

    const std::string blobPath =
        AssetPath::get("ui/blob.png");

    m_blobTexture =
        iconRenderer.loadTexture(blobPath);

    // Иконка показа / скрытия пароля.
    const std::string passwordPeekPath =
        AssetPath::get("ui/signBlue_2.png");

    m_passwordPeekTexture =
        iconRenderer.loadTexture(passwordPeekPath);
}

void MainMenu::setPreviewNickname(
    const std::string& nickname
)
{
    if (m_previewNickname == nickname)
        return;

    m_previewNickname =
        utf8Prefix(nickname, 15);

    // Сбрасываем пароль только при изменении ника.
    m_playerPassword.clear();
    m_passwordInputActive = false;

    m_skinLookupDone = false;
    m_previewSkinId.reset();
}

MainMenu::MenuLayout MainMenu::computeLayout(
    float screenW,
    float screenH
) const
{
    MenuLayout layout{};

    layout.dialogWidth = 710.0f;
    layout.dialogHeight = 600.0f;

    layout.dialogX = screenW * 0.5f;
    layout.dialogY = screenH * 0.5f;

    layout.dialogLeft =
        layout.dialogX - layout.dialogWidth * 0.5f;

    layout.dialogTop =
        layout.dialogY - layout.dialogHeight * 0.5f;

    // ------------------------------------------------------------
    // Панель игрока: цвет + фиксированный блок полей
    // ------------------------------------------------------------

    const float playerInfoY =
        layout.dialogTop + 45.0f;

    const float inputGroupGap = 8.0f;
    const float fieldsGap = 8.0f;

    layout.playerFieldsBlockWidth = 550.0f;

    layout.nameInputHeight = 38.0f;
    layout.passwordInputHeight = 38.0f;

    layout.colorSelectorWidth = 48.0f;
    layout.colorSelectorHeight = 48.0f;

    // Вся группа имеет неизменную ширину.
    const float inputGroupWidth =
        layout.colorSelectorWidth +
        inputGroupGap +
        layout.playerFieldsBlockWidth;

    const float inputGroupLeft =
        layout.dialogLeft + 50.0f;

    // Кнопка выбора цвета.
    layout.colorSelectorX =
        inputGroupLeft +
        layout.colorSelectorWidth * 0.5f;

    layout.colorSelectorY = playerInfoY;

    layout.colorSampleSize = 38.0f;
    layout.colorSampleX = layout.colorSelectorX;
    layout.colorSampleY = layout.colorSelectorY;

    // ------------------------------------------------------------
// Геометрия Nick / Password / CH password
// ------------------------------------------------------------

    const bool isClanhouseSelected =
        menuState.selectedModeIndex >= 0 &&
        menuState.selectedModeIndex <
        static_cast<int>(gameModes.size()) &&
        gameModes[menuState.selectedModeIndex].id == "CLANHOUSE";

    // Левый край блока полей.
    const float fieldsLeft =
        inputGroupLeft +
        layout.colorSelectorWidth +
        inputGroupGap;

    // Количество отображаемых полей.
    const int fieldCount =
        1 +
        (m_passwordInputVisible ? 1 : 0) +
        (isClanhouseSelected ? 1 : 0);

    // Все поля имеют одинаковую ширину.
    const float totalFieldsGap =
        static_cast<float>(fieldCount - 1) * fieldsGap;

    const float fieldWidth =
        (layout.playerFieldsBlockWidth - totalFieldsGap) /
        static_cast<float>(fieldCount);

    // ------------------------------------------------------------
    // Nick
    // ------------------------------------------------------------

    layout.nameInputWidth = fieldWidth;
    layout.nameInputHeight = 38.0f;

    layout.nameInputX =
        fieldsLeft + fieldWidth * 0.5f;

    layout.nameInputY = playerInfoY;

    layout.nameInputTextX =
        layout.nameInputX -
        fieldWidth * 0.5f +
        14.0f;

    // ------------------------------------------------------------
    // Обычный Password
    // ------------------------------------------------------------

    layout.passwordInputHeight = 38.0f;

    if (m_passwordInputVisible)
    {
        layout.passwordInputWidth = fieldWidth;

        layout.passwordInputX =
            fieldsLeft +
            fieldWidth +
            fieldsGap +
            fieldWidth * 0.5f;

        layout.passwordInputY = playerInfoY;

        layout.passwordInputTextX =
            layout.passwordInputX -
            fieldWidth * 0.5f +
            14.0f;
    }
    else
    {
        layout.passwordInputWidth = 0.0f;
        layout.passwordInputX = 0.0f;
        layout.passwordInputY = playerInfoY;
        layout.passwordInputTextX = 0.0f;
    }

    // ------------------------------------------------------------
    // CH password — только для ClanHouse
    // ------------------------------------------------------------

    layout.chPasswordInputHeight = 38.0f;

    if (isClanhouseSelected)
    {
        const int chPasswordIndex =
            m_passwordInputVisible ? 2 : 1;

        layout.chPasswordInputWidth = fieldWidth;

        layout.chPasswordInputX =
            fieldsLeft +
            static_cast<float>(chPasswordIndex) *
            (fieldWidth + fieldsGap) +
            fieldWidth * 0.5f;

        layout.chPasswordInputY = playerInfoY;

        layout.chPasswordInputTextX =
            layout.chPasswordInputX -
            fieldWidth * 0.5f +
            14.0f;
    }
    else
    {
        layout.chPasswordInputWidth = 0.0f;
        layout.chPasswordInputX = 0.0f;
        layout.chPasswordInputY = playerInfoY;
        layout.chPasswordInputTextX = 0.0f;
    }

    // ------------------------------------------------------------
    // Игровые режимы
    // ------------------------------------------------------------

    layout.modeButtonWidth = 140.0f;
    layout.modeButtonHeight = 35.0f;
    layout.modeButtonGap = 3.0f;

    layout.modeIconSize = 26.0f;
    layout.modeIconGap = 6.0f;

    layout.modesLeftX =
        layout.dialogLeft + 155.0f;

    layout.modesTopY =
        layout.dialogTop + 100.0f;

    // ------------------------------------------------------------
    // Список серверов
    // ------------------------------------------------------------

    layout.serverListWidth = 400.0f;
    layout.serverRowHeight = 34.0f;
    layout.serverListGap = 4.0f;
    layout.serverVisibleRows = 20;

    layout.serversX =
        layout.dialogLeft + 450.0f;

    layout.serversY =
        layout.modesTopY - 13.0f;

    layout.serverPaddingX = 12.0f;
    layout.serverPaddingY = 23.0f;

    layout.serverTextX =
        layout.serversX -
        layout.serverListWidth * 0.5f +
        layout.serverPaddingX;

    layout.serverButtonWidth = 350.0f;
    layout.serverButtonHeight = 82.0f;

    // ------------------------------------------------------------
    // Кнопки Play / Spectate
    // ------------------------------------------------------------

    layout.playButtonWidth = 200.0f;
    layout.playButtonHeight = 40.0f;

    layout.spectateButtonWidth =
        layout.playButtonWidth;

    layout.spectateButtonHeight =
        layout.playButtonHeight;

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

const std::string& MainMenu::chPassword() const
{
    return m_chPassword;
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

void MainMenu::appendNicknameText(std::string_view text)
{
    if (!m_nicknameInputActive || text.empty())
        return;

    constexpr std::size_t maxNicknameLength = 15;

    const std::size_t currentLength =
        utf8CharacterCount(m_previewNickname);

    if (currentLength >= maxNicknameLength)
        return;

    const std::string textToAppend = utf8Prefix(
        text,
        maxNicknameLength - currentLength
    );

    if (textToAppend.empty())
        return;

    m_previewNickname += textToAppend;

    // Ник изменился — старый пароль больше не используем.
    m_playerPassword.clear();
    m_passwordInputActive = false;

    // Обновляем предпросмотр скина.
    m_skinLookupDone = false;
    m_previewSkinId.reset();
}

void MainMenu::backspaceNickname()
{
    if (!m_nicknameInputActive || m_previewNickname.empty())
        return;

    std::size_t lastCharacter =
        m_previewNickname.size() - 1;

    while (
        lastCharacter > 0 &&
        isUtf8Continuation(
            static_cast<unsigned char>(
                m_previewNickname[lastCharacter]
                ))
        )
    {
        --lastCharacter;
    }

    m_previewNickname.resize(lastCharacter);

    // Ник изменился — очищаем пароль предыдущего ника.
    m_playerPassword.clear();
    m_passwordInputActive = false;

    m_skinLookupDone = false;
    m_previewSkinId.reset();
}

void MainMenu::finishNicknameEditing()
{
    m_nicknameInputActive = false;
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
    // --------------------------------------------------------
    // Сначала обрабатываем уже открытую панель.
    // Она должна перехватывать клики поверх меню.
    // --------------------------------------------------------

    if (m_colorPicker.handleMouseClick(
        mouseX,
        mouseY,
        screenW,
        screenH
    ))
    {
        m_colorPickerConsumedMouseRelease = true;
        return;
    }

    (void)serverList;

    refreshPasswordVisibility();

    const MenuLayout layout =
        computeLayout(screenW, screenH);

    // --------------------------------------------------------
    // Открытие ColorPicker по клику на цветовой образец.
    // --------------------------------------------------------

    const float colorLeft =
        layout.colorSelectorX -
        layout.colorSelectorWidth * 0.5f;

    const float colorRight =
        layout.colorSelectorX +
        layout.colorSelectorWidth * 0.5f;

    const float colorTop =
        layout.colorSelectorY -
        layout.colorSelectorHeight * 0.5f;

    const float colorBottom =
        layout.colorSelectorY +
        layout.colorSelectorHeight * 0.5f;

    if (
        mouseX >= colorLeft &&
        mouseX <= colorRight &&
        mouseY >= colorTop &&
        mouseY <= colorBottom
        )
    {
        finishTextInput();

        m_colorPicker.open();
        m_colorPickerConsumedMouseRelease = true;

        return;
    }

    // ------------------------------------------------------------
    // Фокус полей Nick / Password / CH password
    // ------------------------------------------------------------

    const bool isClanhouseSelected =
        menuState.selectedModeIndex >= 0 &&
        menuState.selectedModeIndex <
        static_cast<int>(gameModes.size()) &&
        gameModes[menuState.selectedModeIndex].id == "CLANHOUSE";

    // ------------------------------------------------------------
    // CH password — только для ClanHouse
    // ------------------------------------------------------------

    if (isClanhouseSelected)
    {
        const float chPasswordLeft =
            layout.chPasswordInputX -
            layout.chPasswordInputWidth * 0.5f;

        const float chPasswordRight =
            layout.chPasswordInputX +
            layout.chPasswordInputWidth * 0.5f;

        const float chPasswordTop =
            layout.chPasswordInputY -
            layout.chPasswordInputHeight * 0.5f;

        const float chPasswordBottom =
            layout.chPasswordInputY +
            layout.chPasswordInputHeight * 0.5f;

        // ------------------------------------------------------------
        // Кнопка показа / скрытия CH password
        // ------------------------------------------------------------

        const float chEyeLeft =
            chPasswordRight - 37.0f;

        const float chEyeTop =
            layout.chPasswordInputY - 14.0f;

        const float chEyeBottom =
            layout.chPasswordInputY + 14.0f;

        if (
            mouseX >= chEyeLeft &&
            mouseX <= chPasswordRight &&
            mouseY >= chEyeTop &&
            mouseY <= chEyeBottom
            )
        {
            m_chPasswordRevealed = !m_chPasswordRevealed;
            m_textSelectingWithMouse = false;
            return;
        }

        // ------------------------------------------------------------
        // Клик по полю CH password
        // ------------------------------------------------------------

        if (
            mouseX >= chPasswordLeft &&
            mouseX <= chPasswordRight &&
            mouseY >= chPasswordTop &&
            mouseY <= chPasswordBottom
            )
        {
            m_nicknameInputActive = false;
            m_passwordInputActive = false;
            m_chPasswordInputActive = true;

            beginTextSelectionFromMouse(
                mouseX,
                screenW,
                screenH
            );

            return;
        }
    }

    // ------------------------------------------------------------
    // Кнопка показа / скрытия пароля
    // ------------------------------------------------------------

    if (m_passwordInputVisible)
    {
        const float passwordRight =
            layout.passwordInputX +
            layout.passwordInputWidth * 0.5f;

        const float buttonLeft =
            passwordRight - 37.0f;

        const float buttonTop =
            layout.passwordInputY - 14.0f;

        const float buttonBottom =
            layout.passwordInputY + 14.0f;

        if (
            mouseX >= buttonLeft &&
            mouseX <= passwordRight &&
            mouseY >= buttonTop &&
            mouseY <= buttonBottom
            )
        {
            m_passwordRevealed = !m_passwordRevealed;
            m_textSelectingWithMouse = false;
            return;
        }
    }

    // ------------------------------------------------------------
    // Обычный Password
    // ------------------------------------------------------------

    if (m_passwordInputVisible)
    {
        const float passwordLeft =
            layout.passwordInputX -
            layout.passwordInputWidth * 0.5f;

        const float passwordRight =
            layout.passwordInputX +
            layout.passwordInputWidth * 0.5f;

        const float passwordTop =
            layout.passwordInputY -
            layout.passwordInputHeight * 0.5f;

        const float passwordBottom =
            layout.passwordInputY +
            layout.passwordInputHeight * 0.5f;

        if (
            mouseX >= passwordLeft &&
            mouseX <= passwordRight &&
            mouseY >= passwordTop &&
            mouseY <= passwordBottom
            )
        {
            m_nicknameInputActive = false;
            m_chPasswordInputActive = false;
            m_passwordInputActive = true;

            beginTextSelectionFromMouse(
                mouseX,
                screenW,
                screenH
            );

            return;
        }
    }

    // ------------------------------------------------------------
    // Nick
    // ------------------------------------------------------------

    const float nickLeft =
        layout.nameInputX -
        layout.nameInputWidth * 0.5f;

    const float nickRight =
        layout.nameInputX +
        layout.nameInputWidth * 0.5f;

    const float nickTop =
        layout.nameInputY -
        layout.nameInputHeight * 0.5f;

    const float nickBottom =
        layout.nameInputY +
        layout.nameInputHeight * 0.5f;

    if (
        mouseX >= nickLeft &&
        mouseX <= nickRight &&
        mouseY >= nickTop &&
        mouseY <= nickBottom
        )
    {
        m_passwordInputActive = false;
        m_chPasswordInputActive = false;
        m_nicknameInputActive = true;
        beginTextSelectionFromMouse(mouseX, screenW, screenH);
        return;
    }

    // ------------------------------------------------------------
    // Клик вне полей — снять фокус
    // ------------------------------------------------------------

    m_nicknameInputActive = false;
    m_passwordInputActive = false;
    m_chPasswordInputActive = false;
    m_textSelectingWithMouse = false;
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

    const float scissorLeft =
        layout.serversX -
        layout.serverListWidth * 0.5f;

    const float scissorRight =
        layout.serversX +
        layout.serverListWidth * 0.5f;

    const float scissorTop =
        serverListTop;

    const float scissorBottom =
        serverListTop +
        serverListHeight;

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

    if (
        mouseX < scissorLeft ||
        mouseX > scissorRight ||
        mouseY < scissorTop ||
        mouseY > scissorBottom
        )
    {
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

            menuState.selectedServerIndex =
                static_cast<int>(serverIndex);

            if (doubleClick)
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

    if (m_colorPickerConsumedMouseRelease)
    {
        m_colorPickerConsumedMouseRelease = false;
        return;
    }

    if (m_colorPicker.isOpen())
        return;

    m_textSelectingWithMouse = false;

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

    if (m_colorPicker.isOpen())
    {
        draggingServerScrollbar = false;
        return;
    }

    if (!leftButton)
    {
        draggingServerScrollbar = false;
        scrollbarDragOffset = 0.0f;
        return;
    }

    if (m_textSelectingWithMouse)
    {
        updateTextSelectionFromMouse(
            mouseX,
            screenW,
            screenH
        );

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
    if (m_colorPicker.isOpen())
        return;

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

    std::string gateway;

    switch (chosen.dataLoc)
    {
    case 1:
        gateway = "game900";
        break;

    case 2:
        gateway = "game903";
        break;

    case 3:
        gateway = "game906";
        break;

    default:
        return {};
    }

    return "ws://" +
        gateway +
        ".petridish.pw/" +
        chosen.id;
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
    float mouseY,
    bool showBackgroundGrid
)
{
    refreshPasswordVisibility();

    const MenuLayout layout =
        computeLayout(screenW, screenH);

    if (showBackgroundGrid)
    {
        m_backgroundGridRenderer.draw(screenW, screenH);
    }

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

        // Фон — #F8FAFC
        248.0f / 255.0f,
        250.0f / 255.0f,
        252.0f / 255.0f,
        1.0f,

        // Тонкая граница — #CBD5E1
        203.0f / 255.0f,
        213.0f / 255.0f,
        225.0f / 255.0f,
        1.0f,

        // Толщина границы
        1.0f,

        screenW,
        screenH
    );
    
    // ------------------------------------------------------------
    // Выбор цвета / увеличенная подложка под скином
    // ------------------------------------------------------------

    const RGB selectedColor =
        m_colorPicker.selectedColor();

    const float previewR =
        selectedColor.r / 255.0f;

    const float previewG =
        selectedColor.g / 255.0f;

    const float previewB =
        selectedColor.b / 255.0f;

    // Подложка на 14 пикселей больше скина: по 7 с каждой стороны.
    constexpr float previewBackgroundExtra = 14.0f;

    const float backgroundSize =
        layout.colorSampleSize + previewBackgroundExtra;

    if (m_circledBlobTexture != 0)
    {
        iconRenderer.draw(
            m_circledBlobTexture,
            layout.colorSampleX,
            layout.colorSampleY,
            backgroundSize,
            backgroundSize,
            screenW,
            screenH,
            previewR,
            previewG,
            previewB,
            1.0f
        );
    }

    // ------------------------------------------------------------
    // Поиск skinId
    // ------------------------------------------------------------

    if (userSkinDatabase.isLoaded())
    {
        if (!m_skinLookupDone ||
            m_lastSkinLookupNickname != m_previewNickname)
        {
            m_lastSkinLookupNickname =
                m_previewNickname;

            m_previewSkinId =
                userSkinDatabase.findSkinId(
                    m_previewNickname
                );

            m_skinLookupDone = true;
        }
    }

    // ------------------------------------------------------------
    // Получение текстуры
    // ------------------------------------------------------------

    GLuint previewSkinTexture = 0;

    if (m_previewSkinId.has_value())
    {
        previewSkinTexture =
            skinManager.getTexture(*m_previewSkinId);
    }

    // ------------------------------------------------------------
    // Отрисовка круглого скина
    // ------------------------------------------------------------

    if (previewSkinTexture != 0)
    {
        skinShader.use();

        skinShader.setFloat(m_uSkinDepth, 0.0f);

        skinShader.setVec3(
            m_uSkinBaseColor,
            previewR,
            previewG,
            previewB
        );

        skinShader.setVec2(
            m_uSkinCenter,
            layout.colorSampleX,
            layout.colorSampleY
        );

        skinShader.setFloat(
            m_uSkinRadius,
            layout.colorSampleSize * 0.5f
        );

        // SkinShader использует систему экранных координат.
        // Смещением камеры переносим центр координат в центр окна.
        skinShader.setVec2(
            m_uSkinCameraPos,
            screenW * 0.5f,
            screenH * 0.5f
        );

        skinShader.setFloat(
            m_uSkinZoom,
            1.0f
        );

        skinShader.setVec2(
            m_uSkinScreenSize,
            screenW,
            screenH
        );

        skinShader.setVec2(
            m_uSkinViewportSize,
            screenW,
            screenH
        );

        glActiveTexture(GL_TEXTURE0);

        glBindTexture(
            GL_TEXTURE_2D,
            previewSkinTexture
        );

        skinShader.setInt(
            m_uSkinTexture,
            0
        );

        skinMesh.draw();

        glBindTexture(GL_TEXTURE_2D, 0);
    }


    // ------------------------------------------------------------
    // Поля игрока — единый премиальный стиль
    // ------------------------------------------------------------

    const auto drawPlayerInputField =
        [&](float x,
            float y,
            float width,
            float height,
            bool hovered,
            bool focused)
        {
            // Светлый фон поля.
            float bgR = 241.0f / 255.0f;
            float bgG = 245.0f / 255.0f;
            float bgB = 249.0f / 255.0f;

            if (hovered)
            {
                // При наведении — немного голубее.
                bgR = 232.0f / 255.0f;
                bgG = 240.0f / 255.0f;
                bgB = 248.0f / 255.0f;
            }

            if (focused)
            {
                // Активное поле — светло-голубое.
                bgR = 239.0f / 255.0f;
                bgG = 246.0f / 255.0f;
                bgB = 255.0f / 255.0f;
            }

            // Нейтральная серая граница.
            float borderR = 203.0f / 255.0f;
            float borderG = 213.0f / 255.0f;
            float borderB = 225.0f / 255.0f;
            float borderA = 1.0f;
            float borderWidth = 1.0f;

            if (hovered)
            {
                borderR = 148.0f / 255.0f;
                borderG = 163.0f / 255.0f;
                borderB = 184.0f / 255.0f;
                borderA = 1.0f;
            }

            if (focused)
            {
                // Синий акцент активного поля.
                borderR = 67.0f / 255.0f;
                borderG = 94.0f / 255.0f;
                borderB = 143.0f / 255.0f;
                borderA = 1.0f;
                borderWidth = 1.25f;
            }

            uiPanel.draw(
                x,
                y,
                width,
                height,
                10.0f,

                // Фон
                bgR,
                bgG,
                bgB,
                1.0f,

                // Граница
                borderR,
                borderG,
                borderB,
                borderA,
                borderWidth,

                screenW,
                screenH
            );
        };

    // ------------------------------------------------------------
    // Геометрия и состояния полей
    // ------------------------------------------------------------

    const float nickLeft =
        layout.nameInputX -
        layout.nameInputWidth * 0.5f;

    const float nickRight =
        layout.nameInputX +
        layout.nameInputWidth * 0.5f;

    const float nickTop =
        layout.nameInputY -
        layout.nameInputHeight * 0.5f;

    const float nickBottom =
        layout.nameInputY +
        layout.nameInputHeight * 0.5f;

    const bool playerNickFieldHovered =
        mouseX >= nickLeft &&
        mouseX <= nickRight &&
        mouseY >= nickTop &&
        mouseY <= nickBottom;

    drawPlayerInputField(
        layout.nameInputX,
        layout.nameInputY,
        layout.nameInputWidth,
        layout.nameInputHeight,
        playerNickFieldHovered,
        m_nicknameInputActive
    );

    bool playerPasswordFieldHovered = false;

    if (m_passwordInputVisible)
    {
        const float passwordLeft =
            layout.passwordInputX -
            layout.passwordInputWidth * 0.5f;

        const float passwordRight =
            layout.passwordInputX +
            layout.passwordInputWidth * 0.5f;

        const float passwordTop =
            layout.passwordInputY -
            layout.passwordInputHeight * 0.5f;

        const float passwordBottom =
            layout.passwordInputY +
            layout.passwordInputHeight * 0.5f;

        playerPasswordFieldHovered =
            mouseX >= passwordLeft &&
            mouseX <= passwordRight &&
            mouseY >= passwordTop &&
            mouseY <= passwordBottom;

        drawPlayerInputField(
            layout.passwordInputX,
            layout.passwordInputY,
            layout.passwordInputWidth,
            layout.passwordInputHeight,
            playerPasswordFieldHovered,
            m_passwordInputActive
        );
    }

    // ------------------------------------------------------------
    // CH password — только для режима ClanHouse
    // ------------------------------------------------------------

    const bool isClanhouseSelected =
        menuState.selectedModeIndex >= 0 &&
        menuState.selectedModeIndex <
        static_cast<int>(gameModes.size()) &&
        gameModes[menuState.selectedModeIndex].id == "CLANHOUSE";

    bool playerChPasswordFieldHovered = false;

    if (isClanhouseSelected)
    {
        const float chPasswordLeft =
            layout.chPasswordInputX -
            layout.chPasswordInputWidth * 0.5f;

        const float chPasswordRight =
            layout.chPasswordInputX +
            layout.chPasswordInputWidth * 0.5f;

        const float chPasswordTop =
            layout.chPasswordInputY -
            layout.chPasswordInputHeight * 0.5f;

        const float chPasswordBottom =
            layout.chPasswordInputY +
            layout.chPasswordInputHeight * 0.5f;

        playerChPasswordFieldHovered =
            mouseX >= chPasswordLeft &&
            mouseX <= chPasswordRight &&
            mouseY >= chPasswordTop &&
            mouseY <= chPasswordBottom;

        drawPlayerInputField(
            layout.chPasswordInputX,
            layout.chPasswordInputY,
            layout.chPasswordInputWidth,
            layout.chPasswordInputHeight,
            playerChPasswordFieldHovered,
            m_chPasswordInputActive
        );
    }

    // ------------------------------------------------------------
// Подсветка выделенного текста
// ------------------------------------------------------------

    const auto drawInputSelection =
        [&](std::string_view displayText,
            float textX,
            float centerY,
            float fieldHeight,
            bool focused)
        {
            if (!focused || !hasTextSelection())
                return;

            const std::size_t count =
                utf8CharacterCount(displayText);

            const std::size_t first =
                std::min(
                    std::min(m_textCursor, m_textSelectionAnchor),
                    count
                );

            const std::size_t last =
                std::min(
                    std::max(m_textCursor, m_textSelectionAnchor),
                    count
                );

            if (first >= last)
                return;

            // Ширина исходного текста до указанной позиции.
            const auto widthAt =
                [&](std::size_t characterIndex) -> float
                {
                    return uiFonts.menu.measureWidth(
                        utf8Prefix(displayText, characterIndex)
                    );
                };

            const float selectionLeft =
                textX + widthAt(first);

            const float selectionRight =
                textX + widthAt(last);

            const float selectionWidth =
                selectionRight - selectionLeft;

            if (selectionWidth <= 0.0f)
                return;

            // Рисуем фон до текста, чтобы символы оставались видны.
            uiPanel.draw(
                (selectionLeft + selectionRight) * 0.5f,
                centerY,
                selectionWidth,
                fieldHeight - 12.0f,
                2.0f,

                // Полупрозрачный синий фон выделения.
                59.0f / 255.0f,
                130.0f / 255.0f,
                246.0f / 255.0f,
                0.45f,

                // Без границы.
                0.0f,
                0.0f,
                0.0f,
                0.0f,
                0.0f,

                screenW,
                screenH
            );
        };

    // ------------------------------------------------------------
// Отрисовка курсора отдельно от текста
// ------------------------------------------------------------

    const auto drawInputCaret =
        [&](std::string_view displayText,
            float textX,
            float centerY,
            float fieldHeight,
            bool focused)
        {
            if (!focused)
                return;

            const std::size_t cursor =
                std::min(
                    m_textCursor,
                    utf8CharacterCount(displayText)
                );

            // Курсор располагается после нужного количества символов.
            // Сам текст при этом не изменяется.
            const float cursorX =
                textX +
                uiFonts.menu.measureWidth(
                    utf8Prefix(displayText, cursor)
                );

            constexpr float caretWidth = 2.0f;

            uiPanel.draw(
                cursorX + caretWidth * 0.5f,
                centerY,
                caretWidth,
                fieldHeight - 12.0f,
                0.0f,

                // Яркий оранжевый курсор (#FF7A18).
                1.0f,
                122.0f / 255.0f,
                24.0f / 255.0f,
                1.0f,

                // Без границы.
                0.0f,
                0.0f,
                0.0f,
                0.0f,
                0.0f,

                screenW,
                screenH
            );
        };

    // ------------------------------------------------------------
// Кнопка показа / скрытия пароля
// ------------------------------------------------------------

    const auto drawPasswordVisibilityButton =
        [&](float fieldRight, float centerY)
        {
            const float buttonX = fieldRight - 19.0f;

            // Подложка кнопки.
            uiPanel.draw(
                buttonX,
                centerY,
                36.0f,
                28.0f,
                6.0f,

                0.92f, 0.95f, 0.98f, 1.0f,
                0.60f, 0.67f, 0.75f, 1.0f,
                1.0f,

                screenW,
                screenH
            );

            // Контур глаза.
            uiPanel.draw(
                buttonX,
                centerY,
                19.0f,
                11.0f,
                5.5f,

                0.0f, 0.0f, 0.0f, 0.0f,
                0.18f, 0.25f, 0.33f, 1.0f,
                1.4f,

                screenW,
                screenH
            );

            // Зрачок.
            uiPanel.draw(
                buttonX,
                centerY,
                4.5f,
                4.5f,
                2.25f,

                0.18f, 0.25f, 0.33f, 1.0f,
                0.0f, 0.0f, 0.0f, 0.0f,
                0.0f,

                screenW,
                screenH
            );

            // При показе пароля перечёркиваем глаз.
            if (m_passwordRevealed)
            {
                const std::string slash = "/";
                constexpr float scale = 0.8f;

                const float slashWidth =
                    uiFonts.menu.measureWidth(slash) * scale;

                uiFonts.renderer.begin();

                uiFonts.renderer.add(
                    uiFonts.menu,
                    slash,
                    buttonX - slashWidth * 0.5f,
                    uiFonts.menu.topForCenter(centerY),
                    0.85f,
                    0.20f,
                    0.20f,
                    1.0f,
                    scale
                );

                uiFonts.renderer.flush(
                    uiFonts.menu,
                    screenW,
                    screenH
                );
            }
        };

    // ------------------------------------------------------------
    // Никнейм
    // ------------------------------------------------------------

    const bool nicknameEmpty =
        m_previewNickname.empty();

    std::string nicknameDisplay;

    if (nicknameEmpty && !m_nicknameInputActive)
    {
        nicknameDisplay = "Nick";
    }
    else
    {
        nicknameDisplay = m_previewNickname;
    }

    const float nicknameTextColor =
        nicknameEmpty && !m_nicknameInputActive
        ? 130.0f / 255.0f
        : 38.0f / 255.0f;

    drawInputSelection(
        m_previewNickname,
        layout.nameInputTextX,
        layout.nameInputY,
        layout.nameInputHeight,
        m_nicknameInputActive
    );

    uiFonts.renderer.begin();

    uiFonts.renderer.add(
        uiFonts.menu,
        nicknameDisplay,
        layout.nameInputTextX,
        uiFonts.menu.topForCenter(layout.nameInputY),
        nicknameTextColor,
        nicknameTextColor,
        nicknameTextColor,
        1.0f,
        1.0f
    );

    uiFonts.renderer.flush(
        uiFonts.menu,
        screenW,
        screenH
    );

    drawInputCaret(
        nicknameDisplay,
        layout.nameInputTextX,
        layout.nameInputY,
        layout.nameInputHeight,
        m_nicknameInputActive
    );

    // ------------------------------------------------------------
    // Password
    // ------------------------------------------------------------

    if (m_passwordInputVisible)
    {
        std::string passwordDisplay;

        if (m_playerPassword.empty() && !m_passwordInputActive)
        {
            passwordDisplay = "Password";
        }
        else if (m_passwordRevealed)
        {
            passwordDisplay = m_playerPassword;
        }
        else
        {
            passwordDisplay = std::string(
                utf8CharacterCount(m_playerPassword),
                '*'
            );
        }

        const bool passwordPlaceholder =
            m_playerPassword.empty() &&
            !m_passwordInputActive;

        const float passwordTextColor =
            passwordPlaceholder
            ? 130.0f / 255.0f
            : 38.0f / 255.0f;

        const std::string passwordSelectionDisplay =
            m_passwordRevealed
            ? m_playerPassword
            : std::string(
                utf8CharacterCount(m_playerPassword),
                '*'
            );

        drawInputSelection(
            passwordSelectionDisplay,
            layout.passwordInputTextX,
            layout.passwordInputY,
            layout.passwordInputHeight,
            m_passwordInputActive
        );

        uiFonts.renderer.begin();

        uiFonts.renderer.add(
            uiFonts.menu,
            passwordDisplay,
            layout.passwordInputTextX,
            uiFonts.menu.topForCenter(layout.passwordInputY),
            passwordTextColor,
            passwordTextColor,
            passwordTextColor,
            1.0f,
            1.0f
        );

        uiFonts.renderer.flush(
            uiFonts.menu,
            screenW,
            screenH
        );

        drawInputCaret(
            passwordDisplay,
            layout.passwordInputTextX,
            layout.passwordInputY,
            layout.passwordInputHeight,
            m_passwordInputActive
        );

        // ------------------------------------------------------------
// Кнопка показа / скрытия пароля — оригинальная текстура
// ------------------------------------------------------------

        if (m_passwordInputVisible && m_passwordPeekTexture != 0)
        {
            const float passwordRight =
                layout.passwordInputX +
                layout.passwordInputWidth * 0.5f;

            const float eyeCenterX =
                passwordRight - 18.0f;

            // Скрытый пароль — исходный цвет иконки.
            // Открытый пароль — голубой оттенок.
            const float tintR =
                m_passwordRevealed ? 0.60f : 1.0f;

            const float tintG =
                m_passwordRevealed ? 0.78f : 1.0f;

            const float tintB = 1.0f;

            iconRenderer.draw(
                m_passwordPeekTexture,
                eyeCenterX,
                layout.passwordInputY,
                25.0f,
                25.0f,
                screenW,
                screenH,
                tintR,
                tintG,
                tintB,
                1.0f
            );
        }
    }

    // ------------------------------------------------------------
    // CH password — текст и курсор
    // ------------------------------------------------------------

    if (isClanhouseSelected)
    {
        std::string chPasswordDisplay;

        if (m_chPasswordInputActive)
        {
            if (m_chPasswordRevealed)
            {
                chPasswordDisplay = m_chPassword;
            }
            else
            {
                chPasswordDisplay = std::string(
                    utf8CharacterCount(m_chPassword),
                    '*'
                );
            }
        }
        else if (m_chPassword.empty())
        {
            chPasswordDisplay = "CH password";
        }
        else if (m_chPasswordRevealed)
        {
            chPasswordDisplay = m_chPassword;
        }
        else
        {
            chPasswordDisplay = std::string(
                utf8CharacterCount(m_chPassword),
                '*'
            );
        }

        const bool chPasswordPlaceholder =
            m_chPassword.empty() &&
            !m_chPasswordInputActive;

        const float chPasswordTextColor =
            chPasswordPlaceholder
            ? 130.0f / 255.0f
            : 38.0f / 255.0f;

        const std::string chPasswordSelectionDisplay =
            m_chPasswordRevealed
            ? m_chPassword
            : std::string(
                utf8CharacterCount(m_chPassword),
                '*'
            );

        drawInputSelection(
            chPasswordSelectionDisplay,
            layout.chPasswordInputTextX,
            layout.chPasswordInputY,
            layout.chPasswordInputHeight,
            m_chPasswordInputActive
        );

        uiFonts.renderer.begin();

        uiFonts.renderer.add(
            uiFonts.menu,
            chPasswordDisplay,
            layout.chPasswordInputTextX,
            uiFonts.menu.topForCenter(layout.chPasswordInputY),
            chPasswordTextColor,
            chPasswordTextColor,
            chPasswordTextColor,
            1.0f,
            1.0f
        );

        uiFonts.renderer.flush(
            uiFonts.menu,
            screenW,
            screenH
        );

        drawInputCaret(
            chPasswordDisplay,
            layout.chPasswordInputTextX,
            layout.chPasswordInputY,
            layout.chPasswordInputHeight,
            m_chPasswordInputActive
        );
    }

    // ------------------------------------------------------------
// Иконка показа / скрытия CH password
// ------------------------------------------------------------

    if (isClanhouseSelected && m_passwordPeekTexture != 0)
    {
        const float chPasswordRight =
            layout.chPasswordInputX +
            layout.chPasswordInputWidth * 0.5f;

        const float eyeCenterX =
            chPasswordRight - 18.0f;

        const float tintR =
            m_chPasswordRevealed ? 0.60f : 1.0f;

        const float tintG =
            m_chPasswordRevealed ? 0.78f : 1.0f;

        const float tintB = 1.0f;

        iconRenderer.draw(
            m_passwordPeekTexture,
            eyeCenterX,
            layout.chPasswordInputY,
            25.0f,
            25.0f,
            screenW,
            screenH,
            tintR,
            tintG,
            tintB,
            1.0f
        );
    }

    // ------------------------------------------------------------
    // Разделитель верхней панели
    // ------------------------------------------------------------

    uiPanel.draw(
        layout.dialogX,
        layout.dialogTop + 77.0f,
        layout.dialogWidth - 4.0f,
        2.0f,
        0.0f,

        // Разделитель — мягкий серо-голубой.
        203.0f / 255.0f,
        213.0f / 255.0f,
        225.0f / 255.0f,
        0.85f,

        // Без рамки.
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
                54.0f / 255.0f,
                127.0f / 255.0f,
                245.0f / 255.0f,
                0.12f,

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

                // background — светло-серый
                241.0f / 255.0f,
                245.0f / 255.0f,
                249.0f / 255.0f,
                0.45f,

                // border — нейтральный серый
                203.0f / 255.0f,
                213.0f / 255.0f,
                225.0f / 255.0f,
                0.90f,

                // border thickness
                1.0f,

                screenW,
                screenH
            );
        }
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
        serverListTop + serverListHeight * 0.5f,
        layout.serverListWidth,
        serverListHeight,
        12.0f,

        // Фон: почти белый (#FAFBFD)
        250.0f / 255.0f,
        251.0f / 255.0f,
        253.0f / 255.0f,
        1.0f,

        // Рамка: мягкий серо-голубой (#D8E1ED)
        216.0f / 255.0f,
        225.0f / 255.0f,
        237.0f / 255.0f,
        1.0f,

        // Толщина рамки
        1.0f,

        screenW,
        screenH
    );

    uiFonts.renderer.begin();

    const float scissorLeft =
        layout.serversX -
        layout.serverListWidth * 0.5f;

    const float scissorRight =
        layout.serversX +
        layout.serverListWidth * 0.5f;

    const float scissorTop =
        serverListTop;

    const float scissorBottom =
        serverListTop +
        serverListHeight;

    glEnable(GL_SCISSOR_TEST);

    glScissor(
        static_cast<GLint>(std::floor(scissorLeft)),
        static_cast<GLint>(std::floor(screenH - scissorBottom)),
        static_cast<GLsizei>(
            std::ceil(scissorRight) -
            std::floor(scissorLeft)
            ),
        static_cast<GLsizei>(
            std::ceil(scissorBottom) -
            std::floor(scissorTop)
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

        const bool mouseInsideServerList =
            mouseX >= scissorLeft &&
            mouseX <= scissorRight &&
            mouseY >= scissorTop &&
            mouseY <= scissorBottom;

        const bool hovered =
            mouseInsideServerList &&
            mouseX >= left &&
            mouseX <= right &&
            mouseY >= top &&
            mouseY <= bottom;

        // --------------------------------------------------------
// Selected / Hover
// --------------------------------------------------------

        constexpr float serverHighlightInsetX = 4.0f;
        constexpr float serverHighlightInsetY = 2.0f;

        const float highlightWidth =
            layout.serverListWidth - 6.0f -
            serverHighlightInsetX * 2.0f;

        const float highlightHeight =
            layout.serverRowHeight -
            serverHighlightInsetY * 2.0f;

        if (selected)
        {
            // Выбранный сервер — мягкая синяя подсветка.
            uiPanel.draw(
                layout.serversX,
                y,
                highlightWidth,
                highlightHeight,
                18.0f,

                54.0f / 255.0f,
                127.0f / 255.0f,
                245.0f / 255.0f,
                0.16f,

                // Без рамки.
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
            // Наведение — лёгкая серая подсветка.
            uiPanel.draw(
                layout.serversX,
                y,
                highlightWidth,
                highlightHeight,
                18.0f,

                241.0f / 255.0f,
                245.0f / 255.0f,
                249.0f / 255.0f,
                0.45f,

                // Тонкая серая рамка.
                203.0f / 255.0f,
                213.0f / 255.0f,
                225.0f / 255.0f,
                0.90f,

                1.0f,

                screenW,
                screenH
            );
        }

        // --------------------------------------------------------
        // Server text
        // --------------------------------------------------------

        const std::string line =
            liveServerDisplayText(server);

        uiFonts.renderer.add(
            uiFonts.menuModes,
            line,
            layout.serverTextX,
            uiFonts.menuModes.topForCenter(y),
            55.0f / 255.0f,
            65.0f / 255.0f,
            81.0f / 255.0f,
            1.0f,
            1.0f
        );
    }

    uiFonts.renderer.flush(
        uiFonts.menuModes,
        screenW,
        screenH
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

        // Фон: фирменный синий, при наведении светлее.
        playHovered
        ? 67.0f / 255.0f
        : 54.0f / 255.0f,
        playHovered
        ? 139.0f / 255.0f
        : 127.0f / 255.0f,
        playHovered
        ? 255.0f / 255.0f
        : 245.0f / 255.0f,
        1.0f,

        // Тонкая светло-синяя граница.
        playHovered
        ? 112.0f / 255.0f
        : 84.0f / 255.0f,
        playHovered
        ? 169.0f / 255.0f
        : 152.0f / 255.0f,
        1.0f,
        playHovered ? 0.70f : 0.45f,

        1.0f,

        screenW,
        screenH
    );

    // ------------------------------------------------------------
    // Spectate — пока оставляем старый вид
    // ------------------------------------------------------------

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

    const bool spectateHovered =
        mouseX >= spectateLeft &&
        mouseX <= spectateRight &&
        mouseY >= spectateTop &&
        mouseY <= spectateBottom;

    uiPanel.draw(
        spectateX,
        spectateY,
        layout.spectateButtonWidth,
        layout.spectateButtonHeight,
        20.0f,

        // background
        spectateHovered
        ? 38.0f / 255.0f
        : 0.0f,
        spectateHovered
        ? 111.0f / 255.0f
        : 0.0f,
        spectateHovered
        ? 255.0f / 255.0f
        : 0.0f,
        spectateHovered
        ? 1.0f
        : 0.0f,

        // border
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

    uiFonts.renderer.begin();

    uiFonts.renderer.add(
        uiFonts.menu,
        "Play",
        playX - uiFonts.menu.measureWidth("Play") * 0.5f,
        uiFonts.menu.topForCenter(playY),
        1.0f,
        1.0f,
        1.0f,
        1.0f,
        1.0f
    );

    uiFonts.renderer.flush(
        uiFonts.menu,
        screenW,
        screenH
    );

    // ------------------------------------------------------------
    // Текст Spectate
    // ------------------------------------------------------------

    uiFonts.renderer.begin();

    uiFonts.renderer.add(
        uiFonts.menu,
        "Spectate",
        spectateX -
        uiFonts.menu.measureWidth("Spectate") * 0.5f,
        uiFonts.menu.topForCenter(spectateY),
        spectateHovered ? 1.0f : 38.0f / 255.0f,
        spectateHovered ? 1.0f : 111.0f / 255.0f,
        spectateHovered ? 1.0f : 255.0f / 255.0f,
        1.0f,
        1.0f
    );

    uiFonts.renderer.flush(
        uiFonts.menu,
        screenW,
        screenH
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
            // Дорожка: светло-серо-голубой (#E2E8F0)
            226.0f / 255.0f,
            232.0f / 255.0f,
            240.0f / 255.0f,
            0.90f,

            // Без рамки
            0.0f, 0.0f, 0.0f, 0.0f,
            0.0f,
            screenW,
            screenH
        );

        // Ползунок: серо-голубой, при наведении — синий.
        const float thumbR =
            scrollbarHovered
            ? 54.0f / 255.0f
            : 148.0f / 255.0f;

        const float thumbG =
            scrollbarHovered
            ? 127.0f / 255.0f
            : 163.0f / 255.0f;

        const float thumbB =
            scrollbarHovered
            ? 245.0f / 255.0f
            : 184.0f / 255.0f;

        const float thumbAlpha =
            scrollbarHovered ? 0.95f : 0.75f;

        uiPanel.draw(
            scrollbarX,
            scrollbarThumbTop +
            scrollbarThumbHeight * 0.5f,
            kServerScrollbarWidth,
            scrollbarThumbHeight,
            kServerScrollbarThumbRadius,

            // Thumb
            thumbR,
            thumbG,
            thumbB,
            thumbAlpha,

            // Без рамки
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

    uiFonts.renderer.begin();

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

        const std::string& modeName =
            gameModes[i].name;

        const float textR =
            selected ? 54.0f / 255.0f : 51.0f / 255.0f;

        const float textG =
            selected ? 127.0f / 255.0f : 65.0f / 255.0f;

        const float textB =
            selected ? 245.0f / 255.0f : 85.0f / 255.0f;

        uiFonts.renderer.add(
            uiFonts.menuModes,
            modeName,
            x - uiFonts.menuModes.measureWidth(modeName) * 0.5f,
            uiFonts.menuModes.topForCenter(y),
            textR,
            textG,
            textB,
            1.0f,
            1.0f
        );
    }

    uiFonts.renderer.flush(
        uiFonts.menuModes,
        screenW,
        screenH
    );

    // Модальное окно рисуется поверх основного меню.
    m_colorPicker.draw(screenW, screenH);
}

void MainMenu::refreshPasswordVisibility()
{
    if (!userSkinDatabase.isLoaded())
        return;

    const bool shouldBeVisible =
        userSkinDatabase.requiresPassword(m_previewNickname);

    if (shouldBeVisible == m_passwordInputVisible)
        return;

    m_passwordInputVisible = shouldBeVisible;

    // Если новый ник не требует пароля, очищаем его,
    // чтобы случайно не отправить старый пароль.
    if (!shouldBeVisible)
    {
        m_passwordInputActive = false;
        m_playerPassword.clear();
    }
}

void MainMenu::appendPasswordText(std::string_view text)
{
    if (!m_passwordInputActive || text.empty())
        return;

    constexpr std::size_t maxPasswordLength = 10;

    const std::size_t currentLength =
        utf8CharacterCount(m_playerPassword);

    if (currentLength >= maxPasswordLength)
        return;

    m_playerPassword += utf8Prefix(
        text,
        maxPasswordLength - currentLength
    );
}

void MainMenu::backspacePassword()
{
    if (!m_passwordInputActive || m_playerPassword.empty())
        return;

    std::size_t lastCharacter =
        m_playerPassword.size() - 1;

    while (
        lastCharacter > 0 &&
        isUtf8Continuation(
            static_cast<unsigned char>(
                m_playerPassword[lastCharacter]
                ))
        )
    {
        --lastCharacter;
    }

    m_playerPassword.resize(lastCharacter);
}

void MainMenu::appendTextInput(std::string_view text)
{
    replaceTextSelection(text);
}

void MainMenu::backspaceTextInput()
{
    if (!activeTextBuffer())
        return;

    if (hasTextSelection())
    {
        replaceTextSelection({});
        return;
    }

    if (m_textCursor == 0)
        return;

    m_textSelectionAnchor = m_textCursor - 1;

    replaceTextSelection({});
}

std::string* MainMenu::activeTextBuffer()
{
    if (m_nicknameInputActive)
        return &m_previewNickname;

    if (m_passwordInputActive)
        return &m_playerPassword;

    if (m_chPasswordInputActive)
        return &m_chPassword;

    return nullptr;
}

void MainMenu::deleteTextForward()
{
    const std::string* text = activeTextBuffer();

    if (!text)
        return;

    if (hasTextSelection())
    {
        replaceTextSelection({});
        return;
    }

    if (m_textCursor >= utf8CharacterCount(*text))
        return;

    m_textSelectionAnchor = m_textCursor + 1;

    replaceTextSelection({});
}

void MainMenu::moveTextCursorLeft(bool extendSelection)
{
    if (!activeTextBuffer())
        return;

    if (!extendSelection && hasTextSelection())
    {
        m_textCursor =
            std::min(m_textCursor, m_textSelectionAnchor);

        m_textSelectionAnchor = m_textCursor;
        return;
    }

    if (m_textCursor > 0)
        --m_textCursor;

    if (!extendSelection)
        m_textSelectionAnchor = m_textCursor;
}

void MainMenu::moveTextCursorRight(bool extendSelection)
{
    const std::string* text = activeTextBuffer();

    if (!text)
        return;

    if (!extendSelection && hasTextSelection())
    {
        m_textCursor =
            std::max(m_textCursor, m_textSelectionAnchor);

        m_textSelectionAnchor = m_textCursor;
        return;
    }

    m_textCursor = std::min(
        m_textCursor + 1,
        utf8CharacterCount(*text)
    );

    if (!extendSelection)
        m_textSelectionAnchor = m_textCursor;
}

void MainMenu::moveTextCursorHome(bool extendSelection)
{
    if (!activeTextBuffer())
        return;

    m_textCursor = 0;

    if (!extendSelection)
        m_textSelectionAnchor = m_textCursor;
}

void MainMenu::moveTextCursorEnd(bool extendSelection)
{
    const std::string* text = activeTextBuffer();

    if (!text)
        return;

    m_textCursor = utf8CharacterCount(*text);

    if (!extendSelection)
        m_textSelectionAnchor = m_textCursor;
}

std::size_t MainMenu::textIndexFromMouseX(
    float mouseX,
    float screenW,
    float screenH
) const
{
    const MenuLayout layout =
        computeLayout(screenW, screenH);

    float textX = layout.nameInputTextX;

    if (m_passwordInputActive)
        textX = layout.passwordInputTextX;
    else if (m_chPasswordInputActive)
        textX = layout.chPasswordInputTextX;

    const std::string display = activeTextDisplay();
    const std::size_t count = utf8CharacterCount(display);

    const float relativeX = mouseX - textX;

    float previousWidth = 0.0f;

    for (std::size_t i = 1; i <= count; ++i)
    {
        const float nextWidth =
            uiFonts.menu.measureWidth(utf8Prefix(display, i));

        const float midpoint =
            (previousWidth + nextWidth) * 0.5f;

        if (relativeX < midpoint)
            return i - 1;

        previousWidth = nextWidth;
    }

    return count;
}

void MainMenu::beginTextSelectionFromMouse(
    float mouseX,
    float screenW,
    float screenH
)
{
    m_textSelectingWithMouse = true;

    m_textCursor =
        textIndexFromMouseX(mouseX, screenW, screenH);

    m_textSelectionAnchor = m_textCursor;
}

void MainMenu::updateTextSelectionFromMouse(
    float mouseX,
    float screenW,
    float screenH
)
{
    if (!m_textSelectingWithMouse || !activeTextBuffer())
        return;

    // Anchor остаётся на начальной точке клика.
    m_textCursor =
        textIndexFromMouseX(mouseX, screenW, screenH);
}

const std::string* MainMenu::activeTextBuffer() const
{
    if (m_nicknameInputActive)
        return &m_previewNickname;

    if (m_passwordInputActive)
        return &m_playerPassword;

    if (m_chPasswordInputActive)
        return &m_chPassword;

    return nullptr;
}

std::string MainMenu::activeTextDisplay() const
{
    const std::string* text = activeTextBuffer();

    if (!text)
        return {};

    // Пароли отображаются звёздочками, но редактируются
    // по позициям символов исходной строки.
    if (
        (m_passwordInputActive && m_passwordRevealed) ||
        (m_chPasswordInputActive && m_chPasswordRevealed)
        )
    {
        return *text;
    }

    if (m_passwordInputActive || m_chPasswordInputActive)
    {
        return std::string(
            utf8CharacterCount(*text),
            '*'
        );
    }

    return *text;
}

bool MainMenu::hasTextSelection() const
{
    return activeTextBuffer() &&
        m_textCursor != m_textSelectionAnchor;
}

std::string MainMenu::selectedText() const
{
    const std::string* text = activeTextBuffer();

    if (!text || !hasTextSelection())
        return {};

    const std::size_t first =
        std::min(m_textCursor, m_textSelectionAnchor);

    const std::size_t last =
        std::max(m_textCursor, m_textSelectionAnchor);

    const std::size_t startByte =
        utf8ByteOffset(*text, first);

    const std::size_t endByte =
        utf8ByteOffset(*text, last);

    return text->substr(startByte, endByte - startByte);
}

void MainMenu::selectAllText()
{
    const std::string* text = activeTextBuffer();

    if (!text)
        return;

    m_textSelectionAnchor = 0;
    m_textCursor = utf8CharacterCount(*text);
}

void MainMenu::deleteTextSelection()
{
    if (hasTextSelection())
        replaceTextSelection({});
}

void MainMenu::replaceTextSelection(std::string_view text)
{
    std::string* value = activeTextBuffer();

    if (!value)
        return;

    const std::size_t oldLength =
        utf8CharacterCount(*value);

    m_textCursor = std::min(m_textCursor, oldLength);
    m_textSelectionAnchor =
        std::min(m_textSelectionAnchor, oldLength);

    const std::size_t first =
        std::min(m_textCursor, m_textSelectionAnchor);

    const std::size_t last =
        std::max(m_textCursor, m_textSelectionAnchor);

    const std::size_t startByte =
        utf8ByteOffset(*value, first);

    const std::size_t endByte =
        utf8ByteOffset(*value, last);

    // Не допускаем переносов строк и управляющих символов
    // при вставке из системного буфера.
    std::string filtered;
    filtered.reserve(text.size());

    for (unsigned char c : text)
    {
        if (c >= 0x20 && c != 0x7F)
            filtered.push_back(static_cast<char>(c));
    }

    const std::size_t lengthWithoutSelection =
        oldLength - (last - first);

    const std::size_t maximumLength =
        m_nicknameInputActive ? 15 : 10;

    const std::size_t available =
        lengthWithoutSelection < maximumLength
        ? maximumLength - lengthWithoutSelection
        : 0;

    const std::string inserted =
        utf8Prefix(filtered, available);

    const std::string original = *value;

    value->replace(
        startByte,
        endByte - startByte,
        inserted
    );

    m_textCursor =
        first + utf8CharacterCount(inserted);

    m_textSelectionAnchor = m_textCursor;

    if (m_nicknameInputActive && *value != original)
    {
        // Изменился ник — очищаем пароль предыдущего ника.
        m_playerPassword.clear();
        m_passwordInputActive = false;

        m_skinLookupDone = false;
        m_previewSkinId.reset();
    }
}

void MainMenu::finishTextInput()
{
    m_nicknameInputActive = false;
    m_passwordInputActive = false;
    m_chPasswordInputActive = false;
    m_textSelectingWithMouse = false;
}

void MainMenu::setOnPlayerColorSelected(
    std::function<void(int)> callback
)
{
    m_colorPicker.setOnColorSelected(std::move(callback));
}