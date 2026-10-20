#pragma once

#include <glad/glad.h>

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "../Input/InputState.h"
#include "../Core/GameModes.h"
#include "../Graphics/Font.h"
#include "../Graphics/IconRenderer.h"
#include "../Graphics/TextRenderer.h"
#include "../Graphics/UIPanel.h"
#include "../Network/ServerListFetcher.h"
#include "MenuState.h"

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
        TextRenderer& textRenderer,
        IconRenderer& iconRenderer,
        Font& gmFont,
        Font& menuFont,
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
        float mouseY
    );

    void update(
        const std::vector<ServerListEntry>& serverList
    );

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

    bool hasConfirmedSelection(
        const InputState& input,
        const std::vector<ServerListEntry>& serverList
    );

    std::string selectedServerUrl(
        const std::vector<ServerListEntry>& serverList
    ) const;

    const std::string& chPassword() const;

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
    Shader& skinShader;
    SkinMesh& skinMesh;
    SkinManager& skinManager;
    UserSkinDatabase& userSkinDatabase;

    // Uniforms круглого SkinShader
    GLint m_uSkinCenter = -1;
    GLint m_uSkinRadius = -1;
    GLint m_uSkinCameraPos = -1;
    GLint m_uSkinZoom = -1;
    GLint m_uSkinScreenSize = -1;
    GLint m_uSkinTexture = -1;

    // Пока настоящее поле ввода ещё не подключено.
    std::string m_previewNickname = "Android Player";
    std::string m_lastSkinLookupNickname;

    std::optional<uint32_t> m_previewSkinId;
    bool m_skinLookupDone = false;
    bool m_nicknameInputActive = false;
    bool m_passwordInputVisible = false;
    void refreshPasswordVisibility();

    void appendPasswordText(std::string_view text);
    void backspacePassword();

    bool m_passwordInputActive = false;
    std::string m_playerPassword;
    std::string m_chPassword;
    bool m_chPasswordInputActive = false;

};