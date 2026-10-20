#pragma once

#include <cstdint>
#include <vector>
#include <memory>
#include <string>

#include "../../Game/ChatMessage.h"
#include <glad/glad.h>

class UIPanel;
class TextRenderer;
class Font;
class World;
class Shader;
class SkinMesh;
class SkinManager;
class UiFont;
class UiTextRenderer;
class UserSkinDatabase;
class FrameProfiler;
class ChatHUD
{
public:
    ChatHUD(
        UIPanel& uiPanel,
        TextRenderer& textRenderer,
        Font& font,
        World& world,
        Shader& chatSkinShader,
        SkinMesh& skinMesh,
        SkinManager& skinManager,
        UiFont& chatFont,
        UiTextRenderer& chatTextRenderer,
        UserSkinDatabase& userSkinDatabase
    );

    void draw(
        FrameProfiler& profiler,
        const std::vector<uint32_t>& ownedIds,
        float screenW,
        float screenH
    );

private:
    struct ChatVisualMessage
    {
        const ChatMessage* message = nullptr;

        std::string displayName;
        std::vector<std::string> lines;
        std::vector<std::string> enterNameLines;

        bool isEnter = false;
        bool isContinuation = false;
        bool enterTextOnSecondLine = false;
        bool hasVerifiedMark = false;

        float boxHeight = 0.0f;
        float topMargin = 0.0f;
        float bottomMargin = 0.0f;

        // Ширины текста кэшируются вместе с разметкой.
        float enterLastNameWidth = 0.0f;
        float displayNameWidth = 0.0f;
    };

    std::shared_ptr<const std::vector<ChatMessage>>
        preparedMessagesSnapshot;

    std::vector<ChatVisualMessage> visualMessages;

    std::uint64_t preparedChatVersion = 0;
    std::uint64_t preparedChatResetVersion = 0;

    bool hasPreparedChatView = false;
    // Защищённость сообщений уже рассчитана для загруженной базы.
    bool verifiedMarksPrepared = false;

    UIPanel& uiPanel;
    TextRenderer& textRenderer;
    Font& font;
    World& world;
    Shader& chatSkinShader;
    SkinMesh& skinMesh;
    SkinManager& skinManager;
    UiFont& chatFont;
    UiTextRenderer& chatTextRenderer;
    UserSkinDatabase& userSkinDatabase;

    GLint chatSkinCenter = -1;
    GLint chatSkinRadius = -1;
    GLint chatSkinScreenSize = -1;
    GLint chatSkinTexture = -1;
    GLint chatSkinBorderWidth = -1;
    GLint chatSkinBorderColor = -1;
};