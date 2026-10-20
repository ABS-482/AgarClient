#pragma once
#include <glad/glad.h>
#include <ft2build.h>
#include FT_FREETYPE_H

#include <cstdint>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <cstddef>

class UiFont
{
public:
    struct Glyph
    {
        float u0 = 0, v0 = 0, u1 = 0, v1 = 0;
        float w = 0, h = 0;
        float xoff = 0, yoff = 0;
        float advance = 0;
    };

    // fallbackPaths: запасные шрифты по порядку, например {"fonts/arial_unicode.otf"}.
    UiFont(const std::string& ttfPath, float cssPx, float dpiScale = 1.0f,
        float coverageGamma = 1.0f,
        const std::vector<std::string>& fallbackPaths = {});
    ~UiFont();

    UiFont(const UiFont&) = delete;
    UiFont& operator=(const UiFont&) = delete;

    float measureWidth(const std::string& utf8) const;
    float buildQuads(const std::string& utf8, float x, float baselineY,
        std::vector<float>& out) const;

    float topForCenter(float centerY) const
    {
        return centerY - (m_ascent - m_descent) * 0.5f;
    }

    float ascent() const { return m_ascent; }
    float descent() const { return m_descent; }
    float lineHeight() const { return m_ascent - m_descent + m_lineGap; }
    GLuint textureId() const { return m_texture; }

private:
    static constexpr int ATLAS = 2048;   // R8, 4 МБ; хватает на ~15 тыс. CJK-глифов

    const Glyph* find(uint32_t cp) const;
    const Glyph* loadGlyph(uint32_t cp) const;

    struct CachedTextRun
    {
        std::vector<Glyph> glyphs;
        float width = 0.0f;
        std::size_t quadGlyphCount = 0;
    };

    const CachedTextRun& getCachedTextRun(
        const std::string& utf8
    ) const;

    static constexpr std::size_t TEXT_RUN_CACHE_LIMIT = 2048;

    FT_Library m_library = nullptr;
    std::vector<FT_Face> m_faces;        // [0] основной, дальше fallback
    unsigned char m_lut[256];

    // Кэш растёт при рисовании, поэтому mutable.
    // Только из потока с GL-контекстом.
    mutable std::unordered_map<uint32_t, Glyph> m_glyphs;
    mutable std::unordered_set<uint32_t> m_absent;
    mutable std::unordered_map<std::string, CachedTextRun>
        m_textRunCache;
    mutable int m_penX = 1, m_penY = 1, m_rowH = 0;
    mutable bool m_atlasFullLogged = false;

    GLuint m_texture = 0;
    float m_ascent = 0, m_descent = 0, m_lineGap = 0;
};