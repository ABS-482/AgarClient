#pragma once

#include <glad/glad.h>
#include <stb_truetype.h>

#include <string>
#include <vector>

// Один запечённый ASCII-атлас шрифта (символы 32..127).
class Font
{
public:
    Font(const std::string& ttfPath, float pixelHeight);
    ~Font();

    Font(const Font&) = delete;
    Font& operator=(const Font&) = delete;

    GLuint textureId() const { return m_texture; }

    // Строит вершины (x, y, u, v) для строки, начиная с (x, y) —
    // y это базовая линия текста (baseline), как в stb_truetype.
    // Возвращает итоговую ширину строки в пикселях.
    float buildQuads(
        const std::string& text,
        float x, float y,
        std::vector<float>& outVertices
    ) const;

private:
    static constexpr int m_atlasWidth = 512;
    static constexpr int m_atlasHeight = 512;
    static constexpr int m_firstChar = 32;
    static constexpr int m_numChars = 96;

    GLuint m_texture = 0;
    std::vector<stbtt_bakedchar> m_bakedChars;
};