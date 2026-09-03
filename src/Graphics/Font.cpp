#define STB_TRUETYPE_IMPLEMENTATION
#include "Font.h"

#include <fstream>
#include <iostream>

Font::Font(const std::string& ttfPath, float pixelHeight)
{
    std::ifstream file(ttfPath, std::ios::binary | std::ios::ate);

    if (!file)
    {
        std::cerr << "Font: failed to open " << ttfPath << '\n';
        return;
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<unsigned char> ttfBuffer(static_cast<size_t>(size));

    if (!file.read(reinterpret_cast<char*>(ttfBuffer.data()), size))
    {
        std::cerr << "Font: failed to read " << ttfPath << '\n';
        return;
    }

    std::vector<unsigned char> atlasBitmap(
        static_cast<size_t>(m_atlasWidth) * m_atlasHeight
    );

    m_bakedChars.resize(m_numChars);

    int result = stbtt_BakeFontBitmap(
        ttfBuffer.data(), 0,
        pixelHeight,
        atlasBitmap.data(), m_atlasWidth, m_atlasHeight,
        m_firstChar, m_numChars,
        m_bakedChars.data()
    );

    if (result <= 0)
    {
        std::cerr << "Font: stbtt_BakeFontBitmap failed (атлас слишком мал?)\n";
    }

    glGenTextures(1, &m_texture);
    glBindTexture(GL_TEXTURE_2D, m_texture);

    glTexImage2D(
        GL_TEXTURE_2D, 0, GL_RED,
        m_atlasWidth, m_atlasHeight, 0,
        GL_RED, GL_UNSIGNED_BYTE,
        atlasBitmap.data()
    );

    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glBindTexture(GL_TEXTURE_2D, 0);
}

Font::~Font()
{
    if (m_texture)
        glDeleteTextures(1, &m_texture);
}

float Font::buildQuads(
    const std::string& text,
    float x, float y,
    std::vector<float>& outVertices
) const
{
    float startX = x;

    for (char c : text)
    {
        if (c < m_firstChar || c >= m_firstChar + m_numChars)
            continue;

        stbtt_aligned_quad q;

        stbtt_GetBakedQuad(
            const_cast<stbtt_bakedchar*>(m_bakedChars.data()),
            m_atlasWidth, m_atlasHeight,
            c - m_firstChar,
            &x, &y,
            &q,
            1
        );

        outVertices.insert(outVertices.end(), {
            q.x0, q.y0, q.s0, q.t0,
            q.x1, q.y0, q.s1, q.t0,
            q.x1, q.y1, q.s1, q.t1,

            q.x0, q.y0, q.s0, q.t0,
            q.x1, q.y1, q.s1, q.t1,
            q.x0, q.y1, q.s0, q.t1
            });
    }

    return x - startX;
}