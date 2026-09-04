#pragma once

#include "Font.h"
#include "Shader.h"

#include <string>
#include <vector>

class TextRenderer
{
public:
    // Локации uniform'ов кэшируются один раз, привязка к конкретному
    // шейдеру (у вас textShader — один на весь текст, это безопасно).
    explicit TextRenderer(Shader& shader);
    ~TextRenderer();

    TextRenderer(const TextRenderer&) = delete;
    TextRenderer& operator=(const TextRenderer&) = delete;

    void drawCentered(
        const Font& font,
        const std::string& text,
        float screenCenterX, float screenCenterY,
        float screenWidth, float screenHeight,
        float r, float g, float b,
        float fontScale
    );

private:
    Shader& m_shader;

    GLint m_uAtlas;
    GLint m_uScreenSize;
    GLint m_uScale;
    GLint m_uOffset;
    GLint m_uColor;
    GLint m_uBorderColor;
    GLint m_uAlphaMultiplier;

    unsigned int m_vao = 0;
    unsigned int m_vbo = 0;

    std::vector<float> m_vertexScratch; // переиспользуемый буфер, без аллокаций на каждый вызов
};