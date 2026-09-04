#define STB_TRUETYPE_IMPLEMENTATION
#include "Font.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>

namespace
{
    // Морфологическая дилатация — расширяет маску на radius пикселей
    // во все стороны (берём максимум в квадратной окрестности).
    std::vector<unsigned char> dilate(
        const std::vector<unsigned char>& src, int w, int h, int radius
    )
    {
        std::vector<unsigned char> dst(src.size(), 0);

        for (int y = 0; y < h; ++y)
        {
            for (int x = 0; x < w; ++x)
            {
                unsigned char maxVal = 0;

                for (int dy = -radius; dy <= radius; ++dy)
                {
                    int sy = y + dy;
                    if (sy < 0 || sy >= h) continue;

                    for (int dx = -radius; dx <= radius; ++dx)
                    {
                        int sx = x + dx;
                        if (sx < 0 || sx >= w) continue;

                        unsigned char v = src[sy * w + sx];
                        if (v > maxVal) maxVal = v;
                    }
                }

                dst[y * w + x] = maxVal;
            }
        }

        return dst;
    }
}

Font::Font(const std::string& ttfPath, float pixelHeight, float borderPixels)
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

    stbtt_fontinfo fontInfo;

    if (!stbtt_InitFont(&fontInfo, ttfBuffer.data(), 0))
    {
        std::cerr << "Font: stbtt_InitFont failed\n";
        return;
    }

    float scale = stbtt_ScaleForPixelHeight(&fontInfo, pixelHeight);
    int radius = std::max(1, static_cast<int>(std::round(borderPixels)));

    // RG: R — заливка, G — заливка+обводка вместе (расширенная маска).
    std::vector<unsigned char> atlasBitmap(
        static_cast<size_t>(m_atlasWidth) * m_atlasHeight * 2, 0
    );

    int penX = m_padding;
    int penY = m_padding;
    int rowHeight = 0;

    for (int c = m_firstChar; c < m_firstChar + m_numChars; ++c)
    {
        int glyphIndex = stbtt_FindGlyphIndex(&fontInfo, c);

        int advanceWidth = 0, leftBearing = 0;
        stbtt_GetGlyphHMetrics(&fontInfo, glyphIndex, &advanceWidth, &leftBearing);

        GlyphInfo info{};
        info.advance = advanceWidth * scale;

        if (glyphIndex == 0)
        {
            m_glyphs[c] = info;
            continue;
        }

        int rawW = 0, rawH = 0, rawXoff = 0, rawYoff = 0;

        unsigned char* rawBitmap = stbtt_GetGlyphBitmap(
            &fontInfo, scale, scale, glyphIndex, &rawW, &rawH, &rawXoff, &rawYoff
        );

        if (!rawBitmap || rawW <= 0 || rawH <= 0)
        {
            m_glyphs[c] = info;
            if (rawBitmap) stbtt_FreeBitmap(rawBitmap, nullptr);
            continue;
        }

        int paddedW = rawW + radius * 2;
        int paddedH = rawH + radius * 2;

        std::vector<unsigned char> fillPadded(
            static_cast<size_t>(paddedW) * paddedH, 0
        );

        for (int y = 0; y < rawH; ++y)
        {
            for (int x = 0; x < rawW; ++x)
            {
                fillPadded[(y + radius) * paddedW + (x + radius)] =
                    rawBitmap[y * rawW + x];
            }
        }

        stbtt_FreeBitmap(rawBitmap, nullptr);

        std::vector<unsigned char> shape = dilate(fillPadded, paddedW, paddedH, radius);

        if (penX + paddedW + m_padding > m_atlasWidth)
        {
            penX = m_padding;
            penY += rowHeight + m_padding;
            rowHeight = 0;
        }

        for (int y = 0; y < paddedH; ++y)
        {
            for (int x = 0; x < paddedW; ++x)
            {
                int atlasX = penX + x;
                int atlasY = penY + y;

                if (atlasX < m_atlasWidth && atlasY < m_atlasHeight)
                {
                    size_t idx = (static_cast<size_t>(atlasY) * m_atlasWidth + atlasX) * 2;
                    atlasBitmap[idx] = fillPadded[y * paddedW + x]; // R — заливка
                    atlasBitmap[idx + 1] = shape[y * paddedW + x];      // G — заливка+обводка
                }
            }
        }

        info.u0 = static_cast<float>(penX) / m_atlasWidth;
        info.v0 = static_cast<float>(penY) / m_atlasHeight;
        info.u1 = static_cast<float>(penX + paddedW) / m_atlasWidth;
        info.v1 = static_cast<float>(penY + paddedH) / m_atlasHeight;
        info.width = static_cast<float>(paddedW);
        info.height = static_cast<float>(paddedH);
        info.xoff = static_cast<float>(rawXoff - radius);
        info.yoff = static_cast<float>(rawYoff - radius);

        m_glyphs[c] = info;

        rowHeight = std::max(rowHeight, paddedH);
        penX += paddedW + m_padding;
    }

    glGenTextures(1, &m_texture);
    glBindTexture(GL_TEXTURE_2D, m_texture);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glTexImage2D(
        GL_TEXTURE_2D, 0, GL_RG8,
        m_atlasWidth, m_atlasHeight, 0,
        GL_RG, GL_UNSIGNED_BYTE,
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
        auto it = m_glyphs.find(static_cast<int>(c));

        if (it == m_glyphs.end())
            continue;

        const GlyphInfo& g = it->second;

        if (g.width > 0.0f && g.height > 0.0f)
        {
            float x0 = x + g.xoff;
            float y0 = y + g.yoff;
            float x1 = x0 + g.width;
            float y1 = y0 + g.height;

            outVertices.insert(outVertices.end(), {
                x0, y0, g.u0, g.v0,
                x1, y0, g.u1, g.v0,
                x1, y1, g.u1, g.v1,

                x0, y0, g.u0, g.v0,
                x1, y1, g.u1, g.v1,
                x0, y1, g.u0, g.v1
                });
        }

        x += g.advance;
    }

    return x - startX;
}