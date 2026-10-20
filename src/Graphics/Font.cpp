#define STB_TRUETYPE_IMPLEMENTATION
#include "Font.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <utility>

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

Font::Font(const std::string& ttfPath, float pixelHeight,
    float borderPixels, FontScaleMode scaleMode)
    : m_pixelHeight(pixelHeight), m_scaleMode(scaleMode)
{
    std::ifstream file(ttfPath, std::ios::binary | std::ios::ate);
    if (!file) { std::cerr << "Font: failed to open " << ttfPath << '\n'; return; }

    const std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    m_ttf.resize(static_cast<size_t>(size));
    if (!file.read(reinterpret_cast<char*>(m_ttf.data()), size))
    {
        std::cerr << "Font: failed to read " << ttfPath << '\n'; m_ttf.clear(); return;
    }

    if (!stbtt_InitFont(&m_info, m_ttf.data(), 0))
    {
        std::cerr << "Font: stbtt_InitFont failed\n"; m_ttf.clear(); return;
    }

    m_scale = (m_scaleMode == FontScaleMode::Em)
        ? stbtt_ScaleForMappingEmToPixels(&m_info, pixelHeight)
        : stbtt_ScaleForPixelHeight(&m_info, pixelHeight);

    m_radius = std::max(0, static_cast<int>(std::round(borderPixels)));

    // Пустой атлас, глифы дозаливаются через glTexSubImage2D.
    std::vector<unsigned char> zeros(
        static_cast<size_t>(m_atlasWidth) * m_atlasHeight * 2, 0);

    glGenTextures(1, &m_texture);
    glBindTexture(GL_TEXTURE_2D, m_texture);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RG8, m_atlasWidth, m_atlasHeight, 0,
        GL_RG, GL_UNSIGNED_BYTE, zeros.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Стартовый набор, как раньше.
    for (int c = m_firstChar; c < m_firstChar + m_numChars; ++c) loadGlyph(c);
    for (uint32_t c = 0x0410; c <= 0x044F; ++c) loadGlyph(c);
    for (uint32_t c : { 0x0401u, 0x0451u, 0x203Au, 0x221Eu }) loadGlyph(c);

    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);
    m_mipDirty = false;
}

Font::~Font()
{
    if (m_texture) glDeleteTextures(1, &m_texture);
}

const GlyphInfo* Font::loadGlyph(uint32_t cp) const
{
    if (m_ttf.empty()) return nullptr;

    const int glyphIndex = stbtt_FindGlyphIndex(&m_info, static_cast<int>(cp));
    if (glyphIndex == 0) { m_absent.insert(cp); return nullptr; }

    int advanceWidth = 0, leftBearing = 0;
    stbtt_GetGlyphHMetrics(&m_info, glyphIndex, &advanceWidth, &leftBearing);

    GlyphInfo info{};
    info.advance = advanceWidth * m_scale;

    int rawW = 0, rawH = 0, rawXoff = 0, rawYoff = 0;
    unsigned char* raw = stbtt_GetGlyphBitmap(
        &m_info, m_scale, m_scale, glyphIndex, &rawW, &rawH, &rawXoff, &rawYoff);

    if (!raw || rawW <= 0 || rawH <= 0)
    {
        if (raw) stbtt_FreeBitmap(raw, nullptr);
        return &m_glyphs.emplace(static_cast<int>(cp), info).first->second;  // пробел и т.п.
    }

    const int radius = m_radius;
    const int paddedW = rawW + radius * 2;
    const int paddedH = rawH + radius * 2;

    std::vector<unsigned char> fillPadded(static_cast<size_t>(paddedW) * paddedH, 0);
    for (int y = 0; y < rawH; ++y)
        for (int x = 0; x < rawW; ++x)
            fillPadded[(y + radius) * paddedW + (x + radius)] = raw[y * rawW + x];
    stbtt_FreeBitmap(raw, nullptr);

    const std::vector<unsigned char> shape = dilate(fillPadded, paddedW, paddedH, radius);

    if (m_penX + paddedW + m_padding > m_atlasWidth)
    {
        m_penX = m_padding;
        m_penY += m_rowHeight + m_padding;
        m_rowHeight = 0;
    }
    if (m_penY + paddedH + m_padding > m_atlasHeight)
    {
        if (!m_fullLogged) { std::cerr << "Font: glyph atlas is full\n"; m_fullLogged = true; }
        m_absent.insert(cp);       // дальше этот символ рисуется как ›
        return nullptr;
    }

    // R — заливка, G — заливка+обводка.
    std::vector<unsigned char> rg(static_cast<size_t>(paddedW) * paddedH * 2);
    for (int i = 0; i < paddedW * paddedH; ++i)
    {
        rg[i * 2] = fillPadded[i];
        rg[i * 2 + 1] = shape[i];
    }

    GLint prevTex = 0, prevAlign = 4;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &prevTex);
    glGetIntegerv(GL_UNPACK_ALIGNMENT, &prevAlign);
    glBindTexture(GL_TEXTURE_2D, m_texture);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexSubImage2D(GL_TEXTURE_2D, 0, m_penX, m_penY, paddedW, paddedH,
        GL_RG, GL_UNSIGNED_BYTE, rg.data());
    glPixelStorei(GL_UNPACK_ALIGNMENT, prevAlign);
    glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(prevTex));
    m_mipDirty = true;

    info.u0 = static_cast<float>(m_penX) / m_atlasWidth;
    info.v0 = static_cast<float>(m_penY) / m_atlasHeight;
    info.u1 = static_cast<float>(m_penX + paddedW) / m_atlasWidth;
    info.v1 = static_cast<float>(m_penY + paddedH) / m_atlasHeight;
    info.width = static_cast<float>(paddedW);
    info.height = static_cast<float>(paddedH);
    info.xoff = static_cast<float>(rawXoff - radius);
    info.yoff = static_cast<float>(rawYoff - radius);

    m_rowHeight = std::max(m_rowHeight, paddedH);
    m_penX += paddedW + m_padding;

    return &m_glyphs.emplace(static_cast<int>(cp), info).first->second;
}

const GlyphInfo* Font::find(uint32_t cp) const
{
    // Быстрый путь для ASCII, включая уже известные
    // отсутствующие символы и управляющие символы.
    if (cp < m_ascii.size() && m_asciiResolved[cp])
        return m_ascii[cp];

    auto it = m_glyphs.find(static_cast<int>(cp));

    if (it != m_glyphs.end())
    {
        const GlyphInfo* glyph = &it->second;

        if (cp < m_ascii.size())
        {
            m_ascii[cp] = glyph;
            m_asciiResolved[cp] = true;
        }

        return glyph;
    }

    // Сохраняем прежнее поведение для управляющих символов.
    if (cp < 32 || cp == 0x7F)
    {
        if (cp < m_ascii.size())
            m_asciiResolved[cp] = true;

        return nullptr;
    }

    // Ленивая загрузка глифа.
    if (m_absent.find(cp) == m_absent.end())
    {
        if (const GlyphInfo* glyph = loadGlyph(cp))
        {
            if (cp < m_ascii.size())
            {
                m_ascii[cp] = glyph;
                m_asciiResolved[cp] = true;
            }

            return glyph;
        }
    }

    // Сохраняем существующий fallback на символ ›.
    it = m_glyphs.find(0x203A);

    const GlyphInfo* fallback =
        it != m_glyphs.end()
        ? &it->second
        : nullptr;

    if (cp < m_ascii.size())
    {
        m_ascii[cp] = fallback;
        m_asciiResolved[cp] = true;
    }

    return fallback;
}

void Font::updateMipmaps() const
{
    if (!m_mipDirty) return;

    GLint prevTex = 0;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &prevTex);
    glBindTexture(GL_TEXTURE_2D, m_texture);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(prevTex));
    m_mipDirty = false;
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

        const GlyphInfo* gp = find(codepoint);
        if (!gp) continue;
        const GlyphInfo& g = *gp;

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

Font::CachedLayout Font::buildLayout(const std::string& text) const
{
    CachedLayout layout;

    layout.width = buildQuads(
        text,
        0.0f,
        0.0f,
        layout.vertices
    );

    return layout;
}

bool Font::appendTextQuads(
    const std::string& text,
    float screenX,
    float screenY,
    float fontScale,
    float r,
    float g,
    float b,
    float depth,
    bool centerHorizontally,
    bool cacheLayout,
    std::vector<float>& outVertices
) const
{
    if (text.empty())
        return false;

    if (!cacheLayout)
    {
        float textWidth = 0.0f;
        std::size_t drawableGlyphCount = 0;
        std::size_t index = 0;

        // Первый проход: вычисляем ширину и число глифов,
        // для которых действительно нужны вершины.
        while (index < text.size())
        {
            const uint32_t codepoint = decodeUtf8(text, index);
            const GlyphInfo* gp = find(codepoint);

            if (!gp)
                continue;

            textWidth += gp->advance;

            if (gp->width > 0.0f && gp->height > 0.0f)
                ++drawableGlyphCount;
        }

        if (drawableGlyphCount == 0)
            return false;

        if (
            drawableGlyphCount >
            (outVertices.max_size() - outVertices.size()) / 48
            )
        {
            return false;
        }

        const float offsetX =
            centerHorizontally
            ? screenX - (textWidth * fontScale) * 0.5f
            : screenX;

        const float offsetY =
            screenY + (m_pixelHeight * fontScale) / 3.0f;

        const std::size_t base = outVertices.size();

        // На один глиф приходится 6 вершин по 8 float.
        outVertices.resize(base + drawableGlyphCount * 48);

        float* p = outVertices.data() + base;

        const auto writeVertex =
            [&](float x, float y, float u, float v)
            {
                p[0] = x * fontScale + offsetX;
                p[1] = y * fontScale + offsetY;
                p[2] = u;
                p[3] = v;
                p[4] = r;
                p[5] = g;
                p[6] = b;
                p[7] = depth;
                p += 8;
            };

        // Второй проход: напрямую записываем окончательные вершины.
        float penX = 0.0f;
        index = 0;

        while (index < text.size())
        {
            const uint32_t codepoint = decodeUtf8(text, index);
            const GlyphInfo* gp = find(codepoint);

            if (!gp)
                continue;

            const GlyphInfo& g = *gp;

            if (g.width > 0.0f && g.height > 0.0f)
            {
                const float x0 = penX + g.xoff;
                const float y0 = g.yoff;
                const float x1 = x0 + g.width;
                const float y1 = y0 + g.height;

                writeVertex(x0, y0, g.u0, g.v0);
                writeVertex(x1, y0, g.u1, g.v0);
                writeVertex(x1, y1, g.u1, g.v1);

                writeVertex(x0, y0, g.u0, g.v0);
                writeVertex(x1, y1, g.u1, g.v1);
                writeVertex(x0, y1, g.u0, g.v1);
            }

            penX += g.advance;
        }

        return true;
    }

    const CachedLayout* layout = nullptr;
    CachedLayout uncachedLayout;

    auto it = m_layoutCache.find(text);

    if (it != m_layoutCache.end())
    {
        layout = &it->second;
    }
    else
    {
        uncachedLayout = buildLayout(text);

        if (text.size() <= m_maxCachedLayoutTextBytes)
        {
            if (m_layoutCache.size() >= m_layoutCacheLimit)
            {
                const std::string oldestText =
                    m_layoutCacheOrder[m_layoutCacheNext];

                m_layoutCache.erase(oldestText);
                m_layoutCacheOrder[m_layoutCacheNext] = text;

                m_layoutCacheNext =
                    (m_layoutCacheNext + 1) % m_layoutCacheLimit;
            }
            else
            {
                m_layoutCacheOrder.push_back(text);
            }

            auto inserted = m_layoutCache.emplace(
                text,
                std::move(uncachedLayout)
            );

            layout = &inserted.first->second;
        }
        else
        {
            layout = &uncachedLayout;
        }
    }

    if (layout->vertices.empty())
        return false;

    const float offsetX =
        centerHorizontally
        ? screenX - (layout->width * fontScale) * 0.5f
        : screenX;

    const float offsetY =
        screenY + (m_pixelHeight * fontScale) / 3.0f;

    const std::size_t vertexCount =
        layout->vertices.size() / 4;

    if (
        vertexCount >
        (outVertices.max_size() - outVertices.size()) / 8
        )
    {
        return false;
    }

    outVertices.reserve(
        outVertices.size() + vertexCount * 8
    );

    const auto& source = layout->vertices;

    for (std::size_t i = 0; i < source.size(); i += 4)
    {
        outVertices.push_back(
            source[i] * fontScale + offsetX
        );

        outVertices.push_back(
            source[i + 1] * fontScale + offsetY
        );

        outVertices.push_back(source[i + 2]);
        outVertices.push_back(source[i + 3]);

        outVertices.push_back(r);
        outVertices.push_back(g);
        outVertices.push_back(b);
        outVertices.push_back(depth);
    }

    return true;
}

float Font::measureWidth(const std::string& text) const
{
    float x = 0.0f;
    size_t index = 0;

    while (index < text.size())
    {
        const uint32_t codepoint = decodeUtf8(text, index);

        const GlyphInfo* gp = find(codepoint);
        if (!gp)
            continue;

        x += gp->advance;
    }

    return x;
}