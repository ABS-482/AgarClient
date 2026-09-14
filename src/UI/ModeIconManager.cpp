#include "ModeIconManager.h"

#include <stb_image.h>

#include <iostream>

ModeIconManager::~ModeIconManager()
{
    for (const auto& [id, texture] : m_textures)
    {
        if (texture != 0)
            glDeleteTextures(1, &texture);
    }
}

bool ModeIconManager::load(
    const std::string& id,
    const std::string& path
)
{
    int width = 0;
    int height = 0;
    int channels = 0;

    unsigned char* pixels = stbi_load(
        path.c_str(),
        &width,
        &height,
        &channels,
        STBI_rgb_alpha
    );

    if (!pixels)
    {
        std::cerr
            << "ModeIconManager: failed to load "
            << path << '\n';

        return false;
    }

    GLuint texture = 0;

    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGBA8,
        width,
        height,
        0,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        pixels
    );

    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_MIN_FILTER,
        GL_LINEAR
    );

    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_MAG_FILTER,
        GL_LINEAR
    );

    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_WRAP_S,
        GL_CLAMP_TO_EDGE
    );

    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_WRAP_T,
        GL_CLAMP_TO_EDGE
    );

    glBindTexture(GL_TEXTURE_2D, 0);

    stbi_image_free(pixels);

    m_textures[id] = texture;

    return true;
}

GLuint ModeIconManager::getTexture(
    const std::string& id
) const
{
    auto it = m_textures.find(id);

    if (it == m_textures.end())
        return 0;

    return it->second;
}