#include "TextRenderer.h"

#include <glad/glad.h>

#include <vector>

TextRenderer::TextRenderer()
{
    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);

    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

TextRenderer::~TextRenderer()
{
    glDeleteBuffers(1, &m_vbo);
    glDeleteVertexArrays(1, &m_vao);
}

void TextRenderer::drawCentered(
    const Font& font,
    Shader& shader,
    const std::string& text,
    float screenCenterX, float screenCenterY,
    float screenWidth, float screenHeight,
    float r, float g, float b,
    float fontScale
)
{
    std::vector<float> vertices;
    float width = font.buildQuads(text, 0.0f, 0.0f, vertices);

    if (vertices.empty())
        return;

    float scaledWidth = width * fontScale;

    float baseOffsetX = screenCenterX - scaledWidth * 0.5f;
    float baseOffsetY = screenCenterY + (70.0f * fontScale) / 3.0f;

    shader.use();

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, font.textureId());

    shader.setInt(shader.uniformLocation("uAtlas"), 0);
    shader.setVec2(shader.uniformLocation("uScreenSize"), screenWidth, screenHeight);
    shader.setFloat(shader.uniformLocation("uScale"), fontScale);

    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(
        GL_ARRAY_BUFFER,
        vertices.size() * sizeof(float),
        vertices.data(),
        GL_DYNAMIC_DRAW
    );

    // --- Обводка: 4 прохода чёрным цветом со смещением на 1px ---
    constexpr float outlineOffset = 0.6f;

    const float offsets[4][2] =
    {
        { -outlineOffset, 0.0f },
        {  outlineOffset, 0.0f },
        { 0.0f, -outlineOffset },
        { 0.0f,  outlineOffset }
    };

    shader.setVec3(shader.uniformLocation("uColor"), 0.0f, 0.0f, 0.0f);

    for (const auto& [dx, dy] : offsets)
    {
        shader.setVec2(
            shader.uniformLocation("uOffset"),
            baseOffsetX + dx,
            baseOffsetY + dy
        );

        glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(vertices.size() / 4));
    }

    // --- Основной текст поверх обводки ---
    shader.setVec3(shader.uniformLocation("uColor"), r, g, b);
    shader.setVec2(shader.uniformLocation("uOffset"), baseOffsetX, baseOffsetY);

    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(vertices.size() / 4));

    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}