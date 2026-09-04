#pragma once

#include <glad/glad.h>
#include <stb_truetype.h>

#include <string>
#include <unordered_map>
#include <vector>

struct GlyphInfo
{
    float u0 = 0.0f, v0 = 0.0f, u1 = 0.0f, v1 = 0.0f;
    float width = 0.0f, height = 0.0f;
    float xoff = 0.0f, yoff = 0.0f;
    float advance = 0.0f;
};

class Font
{
public:
    // borderPixels — ширина обводки в пикселях запекания (при pixelHeight),
    // как borderWidth=1.0f в Java для BLOB-шрифта.
    Font(const std::string& ttfPath, float pixelHeight, float borderPixels = 1.0f);
    ~Font();

    Font(const Font&) = delete;
    Font& operator=(const Font&) = delete;

    GLuint textureId() const { return m_texture; }

    float buildQuads(
        const std::string& text,
        float x, float y,
        std::vector<float>& outVertices
    ) const;

private:
    static constexpr int m_atlasWidth = 1024;
    static constexpr int m_atlasHeight = 1024;
    static constexpr int m_firstChar = 32;
    static constexpr int m_numChars = 96;
    static constexpr int m_padding = 8;

    GLuint m_texture = 0;
    std::unordered_map<int, GlyphInfo> m_glyphs;
};