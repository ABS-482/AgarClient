
#pragma once

#include <cstdint>
#include <vector>
#include <memory>
#include <string>

#include "../../Game/ChatMessage.h"
#include "../../Graphics/Shader.h"

#include <glad/glad.h>

class UIPanel;
class TextRenderer;
class Font;
class World;
class SkinMesh;
class SkinManager;
class UiFonts;
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
        UiFonts& uiFonts,
        UserSkinDatabase& userSkinDatabase
    );

    ~ChatHUD();

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
    bool verifiedMarksPrepared = false;

    UIPanel& uiPanel;
    TextRenderer& textRenderer;
    Font& font;
    World& world;
    Shader& chatSkinShader;
    Shader verifiedTextureShader;
    SkinMesh& skinMesh;
    SkinManager& skinManager;
    UiFonts& uiFonts;
    UserSkinDatabase& userSkinDatabase;

    GLint chatSkinCenter = -1;
    GLint chatSkinRadius = -1;
    GLint chatSkinScreenSize = -1;
    GLint chatSkinTexture = -1;
    GLint chatSkinBorderWidth = -1;
    GLint chatSkinBorderColor = -1;

    // Текстура verified.
    GLuint verifiedTexture = 0;

    // Uniform-переменные шейдера verified.
    GLint verifiedTextureCenter = -1;
    GLint verifiedTextureHalfSize = -1;
    GLint verifiedTextureScreenSize = -1;
    GLint verifiedTextureSampler = -1;
    GLint verifiedTextureTint = -1;
};