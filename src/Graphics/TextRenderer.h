#pragma once

#include "Font.h"
#include "Shader.h"

#include <string>

class TextRenderer
{
public:
    TextRenderer();
    ~TextRenderer();

    TextRenderer(const TextRenderer&) = delete;
    TextRenderer& operator=(const TextRenderer&) = delete;

    // screenCenterX/Y — центр текста в пикселях экрана.
    void drawCentered(
        const Font& font,
        Shader& shader,
        const std::string& text,
        float screenCenterX, float screenCenterY,
        float screenWidth, float screenHeight,
        float r, float g, float b,
        float fontScale
    );

private:
    unsigned int m_vao = 0;
    unsigned int m_vbo = 0;
};