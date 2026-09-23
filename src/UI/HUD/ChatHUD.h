#pragma once

#include <cstdint>
#include <vector>

#include <glad/glad.h>

class UIPanel;
class TextRenderer;
class Font;
class World;
class Shader;
class SkinMesh;
class SkinManager;

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
        SkinManager& skinManager
    );

    void draw(
        const std::vector<uint32_t>& ownedIds,
        float screenW,
        float screenH
    );

private:
    UIPanel& uiPanel;
    TextRenderer& textRenderer;
    Font& font;
    World& world;
    Shader& chatSkinShader;
    SkinMesh& skinMesh;
    SkinManager& skinManager;

    GLint chatSkinCenter = -1;
    GLint chatSkinRadius = -1;
    GLint chatSkinScreenSize = -1;
    GLint chatSkinTexture = -1;
    GLint chatSkinBorderWidth = -1;
    GLint chatSkinBorderColor = -1;
};
