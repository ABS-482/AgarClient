#pragma once

#include <glad/glad.h>

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>
#include <functional>
#include <cstddef>

#include "../Input/InputState.h"
#include "../Core/GameModes.h"
#include "../Graphics/IconRenderer.h"
#include "../Graphics/UIPanel.h"
#include "../Graphics/BackgroundGridRenderer.h"
#include "../Network/ServerListFetcher.h"
#include "MenuState.h"
#include "../Fonts/UiFonts.h"
#include "ColorPickerPanel.h"

class Shader;
class SkinMesh;
class SkinManager;
class UserSkinDatabase;

class MainMenu
{
public:
    struct MenuLayout
    {
        float dialogX, dialogY, dialogWidth, dialogHeight, dialogLeft, dialogTop;

        float nameInputX, nameInputY;
        float nameInputWidth, nameInputHeight;
        float nameInputTextX;

        float colorSelectorX, colorSelectorY;
        float colorSelectorWidth, colorSelectorHeight;

        float colorSampleX, colorSampleY, colorSampleSize;
        float colorChevronX;

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
        float playerFieldsBlockWidth;
        float passwordInputX, passwordInputY;
        float passwordInputWidth, passwordInputHeight;
        float passwordInputTextX;
        float chPasswordInputX, chPasswordInputY;
        float chPasswordInputWidth, chPasswordInputHeight;
        float chPasswordInputTextX;

        int serverVisibleRows;
        float serverTextX;
    };

    MainMenu(
        UIPanel& uiPanel,
        UiFonts& uiFonts,
        IconRenderer& iconRenderer,
        Shader& skinShader,
        SkinMesh& skinMesh,
        SkinManager& skinManager,
        UserSkinDatabase& userSkinDatabase
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
        float mouseY,
        bool showBackgroundGrid = true
    );

    void update(
        const std::vector<ServerListEntry>& serverList
    );

    int selectedColorIndex() const
    {
        return m_colorPicker.selectedColorIndex();
    }

    void setPreviewNickname(const std::string& nickname);

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

    // Работа с текстовым полем ника
    bool isNicknameInputActive() const
    {
        return m_nicknameInputActive;
    }

    const std::string& nickname() const
    {
        return m_previewNickname;
    }

    void appendNicknameText(std::string_view text);
    void backspaceNickname();
    void finishNicknameEditing();
    void setPasswordVisible(bool visible)
    {
        m_passwordInputVisible = visible;
    }

    // Активно ли какое-либо текстовое поле.
    bool isTextInputActive() const
    {
        return m_nicknameInputActive ||
            m_passwordInputActive ||
            m_chPasswordInputActive;
    }

    // Пароль для передачи при входе.
    const std::string& playerPassword() const
    {
        return m_playerPassword;
    }

    // Ввод в текущее активное поле.
    void appendTextInput(std::string_view text);
    void backspaceTextInput();
    void finishTextInput();

    bool hasTextSelection() const;
    std::string selectedText() const;

    void selectAllText();
    void deleteTextSelection();

    void moveTextCursorLeft(bool extendSelection);
    void moveTextCursorRight(bool extendSelection);
    void moveTextCursorHome(bool extendSelection);
    void moveTextCursorEnd(bool extendSelection);

    void deleteTextForward();

    bool hasConfirmedSelection(
        const InputState& input,
        const std::vector<ServerListEntry>& serverList
    );

    std::string selectedServerUrl(
        const std::vector<ServerListEntry>& serverList
    ) const;

    const std::string& chPassword() const;

    MenuState& state();

    void setOnPlayerColorSelected(
        std::function<void(int)> callback
    );

private:
    MenuLayout computeLayout(float screenW, float screenH) const;

    UIPanel& uiPanel;
    UiFonts& uiFonts;
    IconRenderer& iconRenderer;

    MenuState menuState;
    bool draggingServerScrollbar = false;
    float scrollbarDragOffset = 0.0f;
    float serverScrollOffset = 0.0f;
    Shader& skinShader;
    SkinMesh& skinMesh;
    SkinManager& skinManager;
    UserSkinDatabase& userSkinDatabase;

    ColorPickerPanel m_colorPicker;

    // Не позволяем отпусканию кнопки мыши активировать
    // элемент меню под только что закрытой панелью.
    bool m_colorPickerConsumedMouseRelease = false;

    // Uniforms круглого SkinShader
    GLint m_uSkinCenter = -1;
    GLint m_uSkinBaseColor = -1;
    GLint m_uSkinDepth = -1;
    GLint m_uSkinRadius = -1;
    GLint m_uSkinCameraPos = -1;
    GLint m_uSkinZoom = -1;
    GLint m_uSkinScreenSize = -1;
    GLint m_uSkinTexture = -1;
    GLint m_uSkinViewportSize = -1;
    GLuint m_circledBlobTexture = 0;
    GLuint m_blobTexture = 0;
    GLuint m_passwordPeekTexture = 0;

    BackgroundGridRenderer m_backgroundGridRenderer;

    // Пока настоящее поле ввода ещё не подключено.
    std::string m_previewNickname = "PetriDish.pw";
    std::string m_lastSkinLookupNickname;

    std::optional<uint32_t> m_previewSkinId;
    bool m_skinLookupDone = false;
    bool m_nicknameInputActive = false;
    bool m_passwordInputVisible = false;
    bool m_passwordRevealed = false;
    bool m_chPasswordRevealed = false;
    void refreshPasswordVisibility();

    void appendPasswordText(std::string_view text);
    void backspacePassword();

    bool m_passwordInputActive = false;
    std::string m_playerPassword;
    std::string m_chPassword;
    bool m_chPasswordInputActive = false;

    // Позиции в символах UTF-8, а не в байтах.
    std::size_t m_textCursor = 0;
    std::size_t m_textSelectionAnchor = 0;

    bool m_textSelectingWithMouse = false;

    std::string* activeTextBuffer();
    const std::string* activeTextBuffer() const;

    std::string activeTextDisplay() const;

    void replaceTextSelection(std::string_view text);

    void beginTextSelectionFromMouse(
        float mouseX,
        float screenW,
        float screenH
    );

    void updateTextSelectionFromMouse(
        float mouseX,
        float screenW,
        float screenH
    );

    std::size_t textIndexFromMouseX(
        float mouseX,
        float screenW,
        float screenH
    ) const;

};