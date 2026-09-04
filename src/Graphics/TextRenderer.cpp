#include "TextRenderer.h"

#include <glad/glad.h>

TextRenderer::TextRenderer(Shader& shader)
    : m_shader(shader)
{
    m_uAtlas = shader.uniformLocation("uAtlas");
    m_uScreenSize = shader.uniformLocation("uScreenSize");
    m_uScale = shader.uniformLocation("uScale");
    m_uOffset = shader.uniformLocation("uOffset");
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

    m_vertexScratch.reserve(256);
}

TextRenderer::~TextRenderer()
{
    glDeleteBuffers(1, &m_vbo);
    glDeleteVertexArrays(1, &m_vao);
}

void TextRenderer::drawCentered(
    const Font& font,
    const std::string& text,
    float screenCenterX, float screenCenterY,
    float screenWidth, float screenHeight,
    float r, float g, float b,
    float fontScale
)
{
    m_vertexScratch.clear(); // не освобождает память, только сбрасывает размер

    float width = font.buildQuads(text, 0.0f, 0.0f, m_vertexScratch);

    if (m_vertexScratch.empty())
        return;

    float scaledWidth = width * fontScale;

    float baseOffsetX = screenCenterX - scaledWidth * 0.5f;
    float baseOffsetY = screenCenterY + (70.0f * fontScale) / 3.0f;

    m_shader.use();

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, font.textureId());

    m_shader.setInt(m_uAtlas, 0);
    m_shader.setVec2(m_uScreenSize, screenWidth, screenHeight);
    m_shader.setFloat(m_uScale, fontScale);
    m_shader.setVec2(m_uOffset, baseOffsetX, baseOffsetY);

    m_shader.setVec3(m_uColor, r, g, b);
    m_shader.setVec3(m_uBorderColor, 0.0f, 0.0f, 0.0f);
    m_shader.setFloat(m_uAlphaMultiplier, 1.0f);

    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(
        GL_ARRAY_BUFFER,
        m_vertexScratch.size() * sizeof(float),
        m_vertexScratch.data(),
        GL_DYNAMIC_DRAW
    );

    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(m_vertexScratch.size() / 4));

    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}