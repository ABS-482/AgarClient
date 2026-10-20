#pragma once

#include <glad/glad.h>
#include <stb_truetype.h>

#include <cstdint>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <cstddef>
#include <array>

struct GlyphInfo
{
    float u0 = 0.0f, v0 = 0.0f, u1 = 0.0f, v1 = 0.0f;
    float width = 0.0f, height = 0.0f;
    float xoff = 0.0f, yoff = 0.0f;
    float advance = 0.0f;
};

enum class FontScaleMode { PixelHeight, Em };

class Font
{
public:
    Font(const std::string& ttfPath, float pixelHeight,
        float borderPixels = 1.0f,
        FontScaleMode scaleMode = FontScaleMode::PixelHeight);
    ~Font();

    Font(const Font&) = delete;
    Font& operator=(const Font&) = delete;

    GLuint textureId() const { return m_texture; }

    float buildQuads(const std::string& text, float x, float y,
        std::vector<float>& outVertices) const;
    // Сразу добавляет преобразованные вершины в батч TextRenderer.
    // Формат: x, y, u, v, r, g, b, depth на вершину.
    // centerHorizontally определяет центрирование по X.
    bool appendTextQuads(
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
    ) const;
    float measureWidth(const std::string& text) const;
    float pixelHeight() const { return m_pixelHeight; }

    // Вызвать перед отрисовкой батча: пересобирает mipmaps, если в атлас
    // добавлялись новые глифы.
    void updateMipmaps() const;

private:
    const GlyphInfo* find(uint32_t cp) const;
    struct CachedLayout
    {
        float width = 0.0f;
        std::vector<float> vertices;
    };

    CachedLayout buildLayout(const std::string& text) const;

    static constexpr std::size_t m_layoutCacheLimit = 512;
    static constexpr std::size_t m_maxCachedLayoutTextBytes = 128;

    mutable std::unordered_map<std::string, CachedLayout> m_layoutCache;
    mutable std::vector<std::string> m_layoutCacheOrder;
    mutable std::size_t m_layoutCacheNext = 0;
    const GlyphInfo* loadGlyph(uint32_t cp) const;

    static constexpr int m_atlasWidth = 2048;   // было 1024: 70 px с обводкой + CJK
    static constexpr int m_atlasHeight = 2048;
    static constexpr int m_firstChar = 32;
    static constexpr int m_numChars = 96;
    static constexpr int m_padding = 8;

    GLuint m_texture = 0;
    float m_pixelHeight = 0.0f;
    FontScaleMode m_scaleMode = FontScaleMode::PixelHeight;

    // Данные шрифта живут столько же, сколько объект: stbtt_fontinfo
    // хранит указатель на буфер.
    std::vector<unsigned char> m_ttf;
    stbtt_fontinfo m_info{};
    float m_scale = 0.0f;
    int m_radius = 0;

    // Изменяются при рисовании (ленивая подгрузка).
    mutable std::unordered_map<int, GlyphInfo> m_glyphs;
    mutable std::unordered_set<uint32_t> m_absent;
    mutable std::array<const GlyphInfo*, 128> m_ascii{};
    mutable std::array<bool, 128> m_asciiResolved{};
    mutable int m_penX = m_padding, m_penY = m_padding, m_rowHeight = 0;
    mutable bool m_mipDirty = false;
    mutable bool m_fullLogged = false;
};