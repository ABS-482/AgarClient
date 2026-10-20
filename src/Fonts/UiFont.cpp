#include "UiFont.h"

#include <ft2build.h>
#include FT_FREETYPE_H

#include <algorithm>
#include <cmath>
#include <iostream>
#include <vector>
#include <utility>

namespace
{
    uint32_t nextCodepoint(const std::string& s, size_t& i)
    {
        const unsigned char c = static_cast<unsigned char>(s[i]);

        auto cont = [&s, i](size_t k)
            {
                return i + k < s.size() &&
                    (static_cast<unsigned char>(s[i + k]) & 0xC0) == 0x80;
            };

        auto b = [&s, i](size_t k)
            {
                return static_cast<uint32_t>(
                    static_cast<unsigned char>(s[i + k]) & 0x3F);
            };

        if (c < 0x80)
        {
            ++i;
            return c;
        }

        if ((c & 0xE0) == 0xC0 && cont(1))
        {
            uint32_t r = ((c & 0x1F) << 6) | b(1);
            i += 2;
            return r;
        }

        if ((c & 0xF0) == 0xE0 && cont(1) && cont(2))
        {
            uint32_t r = ((c & 0x0F) << 12) | (b(1) << 6) | b(2);
            i += 3;
            return r;
        }

        if ((c & 0xF8) == 0xF0 && cont(1) && cont(2) && cont(3))
        {
            uint32_t r = ((c & 0x07) << 18) |
                (b(1) << 12) | (b(2) << 6) | b(3);
            i += 4;
            return r;
        }

        ++i;
        return 0xFFFD;
    }

    unsigned char readGrayPixel(
        const FT_Bitmap& bitmap, unsigned int x, unsigned int y)
    {
        // pitch — фактическое количество байт в строке;
        // отрицательное значение означает обратное направление строк.
        const unsigned int pitch =
            static_cast<unsigned int>(std::abs(bitmap.pitch));

        const unsigned int row =
            bitmap.pitch >= 0 ? y : bitmap.rows - 1 - y;

        const unsigned char* src =
            bitmap.buffer + static_cast<size_t>(row) * pitch;

        if (bitmap.pixel_mode == FT_PIXEL_MODE_GRAY)
        {
            const unsigned char value = src[x];

            if (bitmap.num_grays > 1 && bitmap.num_grays != 256)
            {
                return static_cast<unsigned char>(
                    (static_cast<unsigned int>(value) * 255u) /
                    (bitmap.num_grays - 1));
            }

            return value;
        }

        if (bitmap.pixel_mode == FT_PIXEL_MODE_MONO)
        {
            const unsigned char value = src[x >> 3];
            return (value & (0x80 >> (x & 7))) ? 255 : 0;
        }

        return 0;
    }
}

UiFont::UiFont(const std::string& ttfPath, float cssPx, float dpiScale,
    float coverageGamma, const std::vector<std::string>& fallbackPaths)
{
    if (FT_Init_FreeType(&m_library) != 0)
    {
        std::cerr << "UiFont: FT_Init_FreeType failed\n";
        return;
    }

    const float pixelSize = cssPx * dpiScale;

    std::vector<std::string> paths{ ttfPath };
    paths.insert(paths.end(), fallbackPaths.begin(), fallbackPaths.end());

    for (const auto& p : paths)
    {
        FT_Face face = nullptr;
        if (FT_New_Face(m_library, p.c_str(), 0, &face) != 0)
        {
            std::cerr << "UiFont: failed to load font: " << p << '\n';
            if (&p == &paths.front()) return;   // без основного шрифта смысла нет
            continue;
        }

        if (FT_Set_Char_Size(face, 0,
            static_cast<FT_F26Dot6>(std::lround(pixelSize * 64.0f)), 72, 72) != 0)
        {
            std::cerr << "UiFont: FT_Set_Char_Size failed: " << p << '\n';
            FT_Done_Face(face);
            if (&p == &paths.front()) return;
            continue;
        }

        m_faces.push_back(face);
    }

    if (m_faces.empty())
        return;

    // Метрики строки берутся только у основного шрифта.
    FT_Face main = m_faces[0];
    m_ascent = std::round(main->size->metrics.ascender / 64.0f);
    m_descent = std::round(main->size->metrics.descender / 64.0f);
    m_lineGap = main->size->metrics.height / 64.0f - (m_ascent - m_descent);

    for (int i = 0; i < 256; ++i)
        m_lut[i] = static_cast<unsigned char>(
            std::pow(i / 255.0f, coverageGamma) * 255.0f + 0.5f);

    // Пустой атлас (нули), глифы дозаливаются через glTexSubImage2D.
    std::vector<unsigned char> zeros(static_cast<size_t>(ATLAS) * ATLAS, 0);

    glGenTextures(1, &m_texture);
    glBindTexture(GL_TEXTURE_2D, m_texture);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, ATLAS, ATLAS, 0,
        GL_RED, GL_UNSIGNED_BYTE, zeros.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    // Стартовый набор как раньше: чтобы обычный текст не запекался в первом кадре.
    for (uint32_t c = 32; c < 127; ++c) loadGlyph(c);
    for (uint32_t c = 0x0410; c <= 0x044F; ++c) loadGlyph(c);
    for (uint32_t c : { 0x0401u, 0x0451u, 0x203Au, 0x2026u, 0x221Eu }) loadGlyph(c);
}

UiFont::~UiFont()
{
    if (m_texture)
        glDeleteTextures(1, &m_texture);

    for (FT_Face f : m_faces)
        FT_Done_Face(f);

    if (m_library)
        FT_Done_FreeType(m_library);
}

const UiFont::Glyph* UiFont::loadGlyph(uint32_t cp) const
{
    for (FT_Face face : m_faces)
    {
        const FT_UInt gi = FT_Get_Char_Index(face, cp);
        if (gi == 0)
            continue;   // нет в этом шрифте, пробуем следующий

        if (FT_Load_Glyph(face, gi, FT_LOAD_DEFAULT | FT_LOAD_TARGET_LIGHT) != 0)
            continue;
        if (FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL) != 0)
            continue;

        const FT_GlyphSlot slot = face->glyph;
        const FT_Bitmap& bitmap = slot->bitmap;

        Glyph g;
        g.advance = slot->advance.x / 64.0f;
        g.xoff = static_cast<float>(slot->bitmap_left);
        g.yoff = -static_cast<float>(slot->bitmap_top);
        g.w = static_cast<float>(bitmap.width);
        g.h = static_cast<float>(bitmap.rows);

        if (bitmap.width > 0 && bitmap.rows > 0 && bitmap.buffer)
        {
            if (bitmap.pixel_mode != FT_PIXEL_MODE_GRAY &&
                bitmap.pixel_mode != FT_PIXEL_MODE_MONO)
                continue;

            const int w = static_cast<int>(bitmap.width);
            const int h = static_cast<int>(bitmap.rows);
            const int pad = 1;

            if (m_penX + w + pad > ATLAS)
            {
                m_penX = pad;
                m_penY += m_rowH + pad;
                m_rowH = 0;
            }

            if (m_penY + h + pad > ATLAS)
            {
                if (!m_atlasFullLogged)
                {
                    std::cerr << "UiFont: glyph atlas is full\n";
                    m_atlasFullLogged = true;
                }
                break;   // место кончилось, символ станет «отсутствующим»
            }

            std::vector<unsigned char> pixels(static_cast<size_t>(w) * h);
            for (int y = 0; y < h; ++y)
                for (int x = 0; x < w; ++x)
                    pixels[static_cast<size_t>(y) * w + x] = m_lut[readGrayPixel(
                        bitmap, static_cast<unsigned>(x), static_cast<unsigned>(y))];

            // Не ломаем GL-состояние вызывающего кода.
            GLint prevTex = 0, prevAlign = 4;
            glGetIntegerv(GL_TEXTURE_BINDING_2D, &prevTex);
            glGetIntegerv(GL_UNPACK_ALIGNMENT, &prevAlign);

            glBindTexture(GL_TEXTURE_2D, m_texture);
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
            glTexSubImage2D(GL_TEXTURE_2D, 0, m_penX, m_penY, w, h,
                GL_RED, GL_UNSIGNED_BYTE, pixels.data());

            glPixelStorei(GL_UNPACK_ALIGNMENT, prevAlign);
            glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(prevTex));

            g.u0 = static_cast<float>(m_penX) / ATLAS;
            g.v0 = static_cast<float>(m_penY) / ATLAS;
            g.u1 = static_cast<float>(m_penX + w) / ATLAS;
            g.v1 = static_cast<float>(m_penY + h) / ATLAS;

            m_rowH = std::max(m_rowH, h);
            m_penX += w + pad;
        }

        return &m_glyphs.emplace(cp, g).first->second;
    }

    m_absent.insert(cp);
    return nullptr;
}

const UiFont::Glyph* UiFont::find(uint32_t cp) const
{
    auto it = m_glyphs.find(cp);
    if (it != m_glyphs.end())
        return &it->second;

    // Управляющие символы (\n, \t...) не рисуем.
    if (cp < 32 || cp == 0x7F)
        return nullptr;

    if (m_absent.find(cp) == m_absent.end())
    {
        if (const Glyph* g = loadGlyph(cp))
            return g;
    }

    // Отсутствует везде: как в Java (missingGlyph), рисуем ›.
    it = m_glyphs.find(0x203A);
    if (it == m_glyphs.end())
        it = m_glyphs.find('?');
    return it != m_glyphs.end() ? &it->second : nullptr;
}

const UiFont::CachedTextRun&
UiFont::getCachedTextRun(const std::string& utf8) const
{
    const auto cached = m_textRunCache.find(utf8);

    if (cached != m_textRunCache.end())
        return cached->second;

    CachedTextRun run;
    run.glyphs.reserve(
        (std::min)(utf8.size(), std::size_t{ 128 })
    );

    for (std::size_t i = 0; i < utf8.size();)
    {
        const std::uint32_t cp = nextCodepoint(utf8, i);
        const Glyph* glyph = find(cp);

        if (!glyph)
            continue;

        run.glyphs.push_back(*glyph);
        run.width += glyph->advance;

        if (glyph->w > 0.0f && glyph->h > 0.0f)
            ++run.quadGlyphCount;
    }

    // Ограничиваем память, используемую кэшем строк.
    if (m_textRunCache.size() >= TEXT_RUN_CACHE_LIMIT)
        m_textRunCache.clear();

    return m_textRunCache.emplace(
        utf8,
        std::move(run)
    ).first->second;
}

float UiFont::measureWidth(const std::string& s) const
{
    return getCachedTextRun(s).width;
}

float UiFont::buildQuads(
    const std::string& s,
    float x,
    float baselineY,
    std::vector<float>& out
) const
{
    const CachedTextRun& run = getCachedTextRun(s);

    const float startX = x;
    const float by = std::round(baselineY);

    constexpr std::size_t floatsPerQuad = 24;

    const std::size_t originalSize = out.size();

    out.resize(
        originalSize + run.quadGlyphCount * floatsPerQuad
    );

    float* destination =
        run.quadGlyphCount > 0
        ? out.data() + originalSize
        : nullptr;

    for (const Glyph& glyph : run.glyphs)
    {
        if (glyph.w > 0.0f && glyph.h > 0.0f)
        {
            const float x0 = std::round(x) + glyph.xoff;
            const float y0 = by + glyph.yoff;
            const float x1 = x0 + glyph.w;
            const float y1 = y0 + glyph.h;

            // Первый треугольник.
            *destination++ = x0;
            *destination++ = y0;
            *destination++ = glyph.u0;
            *destination++ = glyph.v0;

            *destination++ = x1;
            *destination++ = y0;
            *destination++ = glyph.u1;
            *destination++ = glyph.v0;

            *destination++ = x1;
            *destination++ = y1;
            *destination++ = glyph.u1;
            *destination++ = glyph.v1;

            // Второй треугольник.
            *destination++ = x0;
            *destination++ = y0;
            *destination++ = glyph.u0;
            *destination++ = glyph.v0;

            *destination++ = x1;
            *destination++ = y1;
            *destination++ = glyph.u1;
            *destination++ = glyph.v1;

            *destination++ = x0;
            *destination++ = y1;
            *destination++ = glyph.u0;
            *destination++ = glyph.v1;
        }

        // Сохраняем исходный порядок накопления ширины.
        x += glyph.advance;
    }

    return x - startX;
}