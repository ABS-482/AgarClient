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

    uint32_t decodeUtf8(const std::string& text, size_t& index)
    {
        const unsigned char c0 = static_cast<unsigned char>(text[index]);

        if (c0 < 0x80)
        {
            ++index;
            return c0;
        }

        if ((c0 & 0xE0) == 0xC0 && index + 1 < text.size())
        {
            const unsigned char c1 =
                static_cast<unsigned char>(text[index + 1]);

            if ((c1 & 0xC0) == 0x80)
            {
                index += 2;
                return
                    ((static_cast<uint32_t>(c0) & 0x1F) << 6) |
                    (static_cast<uint32_t>(c1) & 0x3F);
            }
        }

        if ((c0 & 0xF0) == 0xE0 && index + 2 < text.size())
        {
            const unsigned char c1 =
                static_cast<unsigned char>(text[index + 1]);
            const unsigned char c2 =
                static_cast<unsigned char>(text[index + 2]);

            if ((c1 & 0xC0) == 0x80 &&
                (c2 & 0xC0) == 0x80)
            {
                index += 3;
                return
                    ((static_cast<uint32_t>(c0) & 0x0F) << 12) |
                    ((static_cast<uint32_t>(c1) & 0x3F) << 6) |
                    (static_cast<uint32_t>(c2) & 0x3F);
            }
        }

        if ((c0 & 0xF8) == 0xF0 && index + 3 < text.size())
        {
            const unsigned char c1 =
                static_cast<unsigned char>(text[index + 1]);
            const unsigned char c2 =
                static_cast<unsigned char>(text[index + 2]);
            const unsigned char c3 =
                static_cast<unsigned char>(text[index + 3]);

            if ((c1 & 0xC0) == 0x80 &&
                (c2 & 0xC0) == 0x80 &&
                (c3 & 0xC0) == 0x80)
            {
                index += 4;
                return
                    ((static_cast<uint32_t>(c0) & 0x07) << 18) |
                    ((static_cast<uint32_t>(c1) & 0x3F) << 12) |
                    ((static_cast<uint32_t>(c2) & 0x3F) << 6) |
                    (static_cast<uint32_t>(c3) & 0x3F);
            }
        }

        // Некорректный UTF-8 байт.
        ++index;
        return 0;
    }
}

Font::Font(
    const std::string& ttfPath,
    float pixelHeight,
    float borderPixels,
    FontScaleMode scaleMode
)
    : m_pixelHeight(pixelHeight),
    m_scaleMode(scaleMode)
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

    float scale = 0.0f;

    if (m_scaleMode == FontScaleMode::Em)
    {
        scale =
            stbtt_ScaleForMappingEmToPixels(
                &fontInfo,
                pixelHeight
            );
    }
    else
    {
        scale =
            stbtt_ScaleForPixelHeight(
                &fontInfo,
                pixelHeight
            );
    }
    int radius = std::max(0, static_cast<int>(std::round(borderPixels)));

    // RG: R — заливка, G — заливка+обводка вместе (расширенная маска).
    std::vector<unsigned char> atlasBitmap(
        static_cast<size_t>(m_atlasWidth) * m_atlasHeight * 2, 0
    );

    int penX = m_padding;
    int penY = m_padding;
    int rowHeight = 0;

    std::vector<uint32_t> codepoints;

    for (int c = m_firstChar; c < m_firstChar + m_numChars; ++c)
        codepoints.push_back(static_cast<uint32_t>(c));

    // Кириллица, как в Java-версии.
    for (uint32_t c = 0x0410; c <= 0x042F; ++c) // А-Я
        codepoints.push_back(c);

    for (uint32_t c = 0x0430; c <= 0x044F; ++c) // а-я
        codepoints.push_back(c);

    codepoints.push_back(0x0401); // Ё
    codepoints.push_back(0x0451); // ё
    codepoints.push_back(0x203A); // ›
    codepoints.push_back(0x221E); // ∞
    for (uint32_t c : codepoints)
    {
        int glyphIndex = stbtt_FindGlyphIndex(
            &fontInfo,
            static_cast<int>(c)
        );

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

    size_t index = 0;

    while (index < text.size())
    {
        uint32_t codepoint = decodeUtf8(text, index);

        auto it = m_glyphs.find(static_cast<int>(codepoint));

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

float Font::measureWidth(const std::string& text) const
{
    float x = 0.0f;
    size_t index = 0;

    while (index < text.size())
    {
        uint32_t codepoint = decodeUtf8(text, index);

        auto it = m_glyphs.find(static_cast<int>(codepoint));

        if (it == m_glyphs.end())
            continue;

        x += it->second.advance;
    }

    return x;
}