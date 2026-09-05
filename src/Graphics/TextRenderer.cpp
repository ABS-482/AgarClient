#include "TextRenderer.h"

#include <glad/glad.h>

TextRenderer::TextRenderer(Shader& shader)
    : m_shader(shader)
{
    m_uAtlas = shader.uniformLocation("uAtlas");
    m_uScreenSize = shader.uniformLocation("uScreenSize");
    m_uColor = shader.uniformLocation("uColor");
    m_uBorderColor = shader.uniformLocation("uBorderColor");
    m_uAlphaMultiplier = shader.uniformLocation("uAlphaMultiplier");

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

    m_batch.reserve(4096);
    m_scratch.reserve(256);
}

TextRenderer::~TextRenderer()
{
    glDeleteBuffers(1, &m_vbo);
    glDeleteVertexArrays(1, &m_vao);
}

void TextRenderer::begin()
{
    m_batch.clear();
}

void TextRenderer::addText(
    const Font& font,
    const std::string& text,
    float screenCenterX, float screenCenterY,
    float fontScale
)
{
    m_scratch.clear();

    float width = font.buildQuads(text, 0.0f, 0.0f, m_scratch);

    if (m_scratch.empty())
        return;

    float offsetX = screenCenterX - (width * fontScale) * 0.5f;
    float offsetY = screenCenterY + (70.0f * fontScale) / 3.0f;

    // Переносим локальные (baseline-относительные) квады в абсолютные
    // экранные координаты прямо сейчас, на CPU — раз навсегда для этой строки.
    for (size_t i = 0; i < m_scratch.size(); i += 4)
    {
        float localX = m_scratch[i];
        float localY = m_scratch[i + 1];
        float u = m_scratch[i + 2];
        float v = m_scratch[i + 3];

        m_batch.push_back(localX * fontScale + offsetX);
        m_batch.push_back(localY * fontScale + offsetY);
        m_batch.push_back(u);
        m_batch.push_back(v);
    }
}

void TextRenderer::end(
    const Font& font,
    float screenWidth, float screenHeight,
    float r, float g, float b
)
{
    if (m_batch.empty())
        return;

    m_shader.use();

    m_shader.setInt(m_uAtlas, 0);
    m_shader.setVec2(m_uScreenSize, screenWidth, screenHeight);
    m_shader.setVec3(m_uColor, r, g, b);
    m_shader.setVec3(m_uBorderColor, 0.0f, 0.0f, 0.0f);
    m_shader.setFloat(m_uAlphaMultiplier, 1.0f);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, font.textureId()); // <- см. ниже про параметр font
    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(
        GL_ARRAY_BUFFER,
        m_batch.size() * sizeof(float),
        m_batch.data(),
        GL_DYNAMIC_DRAW
    );

    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(m_batch.size() / 4));

    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}