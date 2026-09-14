#include "IconRenderer.h"

#include "Shaders/IconShader.h"

#include <stb_image.h>

#include <iostream>

IconRenderer::IconRenderer()
    : m_shader(IconShader::vertex, IconShader::fragment)
{
    m_uCenter = m_shader.uniformLocation("uCenter");
    m_uHalfSize = m_shader.uniformLocation("uHalfSize");
    m_uScreenSize = m_shader.uniformLocation("uScreenSize");
    m_uIcon = m_shader.uniformLocation("uIcon");

    float quad[] =
    {
        -1.0f, -1.0f,
         1.0f, -1.0f,
         1.0f,  1.0f,

        -1.0f, -1.0f,
         1.0f,  1.0f,
        -1.0f,  1.0f
    };

    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);

    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

IconRenderer::~IconRenderer()
{
    glDeleteBuffers(1, &m_vbo);
    glDeleteVertexArrays(1, &m_vao);
}

GLuint IconRenderer::loadTexture(const std::string& path)
{
    int width = 0, height = 0, channels = 0;

    unsigned char* pixels = stbi_load(path.c_str(), &width, &height, &channels, 4);

    if (!pixels)
    {
        std::cerr << "IconRenderer: failed to load " << path << '\n';
        return 0;
    }

    GLuint texture = 0;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glTexImage2D(
        GL_TEXTURE_2D, 0, GL_RGBA8,
        width, height, 0,
        GL_RGBA, GL_UNSIGNED_BYTE,
        pixels
    );

    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glBindTexture(GL_TEXTURE_2D, 0);

    stbi_image_free(pixels);

    return texture;
}

void IconRenderer::draw(
    GLuint texture,
    float centerX, float centerY,
    float width, float height,
    float screenWidth, float screenHeight
)
{
    if (texture == 0)
        return;

    m_shader.use();

    m_shader.setVec2(m_uCenter, centerX, centerY);
    m_shader.setVec2(m_uHalfSize, width * 0.5f, height * 0.5f);
    m_shader.setVec2(m_uScreenSize, screenWidth, screenHeight);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    m_shader.setInt(m_uIcon, 0);

    glBindVertexArray(m_vao);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}