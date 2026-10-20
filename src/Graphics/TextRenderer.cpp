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
    m_uUseVertexColor = shader.uniformLocation("uUseVertexColor");

    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);

    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);

    constexpr GLsizei stride = 8 * sizeof(float);

    glVertexAttribPointer(
        0,
        2,
        GL_FLOAT,
        GL_FALSE,
        stride,
        (void*)0
    );
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(
        1,
        2,
        GL_FLOAT,
        GL_FALSE,
        stride,
        (void*)(2 * sizeof(float))
    );
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(
        2,
        3,
        GL_FLOAT,
        GL_FALSE,
        stride,
        (void*)(4 * sizeof(float))
    );
    glEnableVertexAttribArray(2);

    glVertexAttribPointer(
        3,
        1,
        GL_FLOAT,
        GL_FALSE,
        stride,
        (void*)(7 * sizeof(float))
    );
    glEnableVertexAttribArray(3);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    m_batch.reserve(4096);
}

TextRenderer::~TextRenderer()
{
    glDeleteBuffers(1, &m_vbo);
    glDeleteVertexArrays(1, &m_vao);
}

void TextRenderer::begin()
{
    m_batch.clear();
    m_hasVertexColors = false;
}

void TextRenderer::addText(
    const Font& font,
    const std::string& text,
    float screenCenterX,
    float screenCenterY,
    float fontScale,
    float depth,
    bool cacheLayout
)
{
    font.appendTextQuads(
        text,
        screenCenterX,
        screenCenterY,
        fontScale,
        1.0f,
        1.0f,
        1.0f,
        depth,
        true,
        cacheLayout,
        m_batch
    );
}

void TextRenderer::addTextColored(
    const Font& font,
    const std::string& text,
    float screenCenterX,
    float screenCenterY,
    float fontScale,
    float r,
    float g,
    float b,
    float depth
)
{
    if (font.appendTextQuads(
        text,
        screenCenterX,
        screenCenterY,
        fontScale,
        r,
        g,
        b,
        depth,
        true,
        true,
        m_batch
    ))
    {
        m_hasVertexColors = true;
    }
}

void TextRenderer::flush(
    const Font& font,
    float screenWidth, float screenHeight,
    float r, float g, float b
)
{
    if (m_batch.empty())
        return;

    m_shader.use();

    m_shader.setInt(m_uAtlas, 0);
    m_shader.setVec2(
        m_uScreenSize,
        screenWidth,
        screenHeight
    );

    m_shader.setVec3(
        m_uColor,
        r, g, b
    );

    m_shader.setVec3(
        m_uBorderColor,
        0.0f,
        0.0f,
        0.0f
    );

    m_shader.setFloat(
        m_uAlphaMultiplier,
        1.0f
    );

    m_shader.setInt(
        m_uUseVertexColor,
        m_hasVertexColors ? 1 : 0
    );

    font.updateMipmaps();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(
        GL_TEXTURE_2D,
        font.textureId()
    );

    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);

    glBufferData(
        GL_ARRAY_BUFFER,
        m_batch.size() * sizeof(float),
        m_batch.data(),
        GL_DYNAMIC_DRAW
    );

    glDrawArrays(
        GL_TRIANGLES,
        0,
        static_cast<GLsizei>(
            m_batch.size() / 8
            )
    );

    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);

    m_batch.clear();
    m_hasVertexColors = false;
}

void TextRenderer::end(
    const Font& font,
    float screenWidth, float screenHeight,
    float r, float g, float b
)
{
    flush(
        font,
        screenWidth,
        screenHeight,
        r,
        g,
        b
    );
}

void TextRenderer::addTextLeftAligned(
    const Font& font,
    const std::string& text,
    float screenLeftX,
    float screenY,
    float fontScale,
    float depth
)
{
    font.appendTextQuads(
        text,
        screenLeftX,
        screenY,
        fontScale,
        1.0f,
        1.0f,
        1.0f,
        depth,
        false,
        true,
        m_batch
    );
}

void TextRenderer::addTextLeftAlignedColored(
    const Font& font,
    const std::string& text,
    float screenLeftX,
    float screenY,
    float fontScale,
    float r,
    float g,
    float b,
    float depth
)
{
    if (font.appendTextQuads(
        text,
        screenLeftX,
        screenY,
        fontScale,
        r,
        g,
        b,
        depth,
        false,
        true,
        m_batch
    ))
    {
        m_hasVertexColors = true;
    }
}