#pragma once

#include "Font.h"
#include "Shader.h"

#include <string>
#include <vector>

class TextRenderer
{
public:
    explicit TextRenderer(Shader& shader);
    ~TextRenderer();

    TextRenderer(const TextRenderer&) = delete;
    TextRenderer& operator=(const TextRenderer&) = delete;

    // Вызвать один раз в начале кадра — очищает накопленный батч.
    void begin();

    // Добавляет квады строки в общий батч. НИЧЕГО не рисует сразу.
    void addText(
        const Font& font,
        const std::string& text,
        float screenCenterX, float screenCenterY,
        float fontScale
    );

    // Один-единственный draw call на ВЕСЬ накопленный за кадр текст.
    void end(
        const Font& font,
        float screenWidth,
        float screenHeight,
        float r, float g, float b
    );

private:
    Shader& m_shader;

    GLint m_uAtlas;
    GLint m_uScreenSize;
    GLint m_uColor;
    GLint m_uBorderColor;
    GLint m_uAlphaMultiplier;

    unsigned int m_vao = 0;
    unsigned int m_vbo = 0;

    std::vector<float> m_batch;   // общий буфер за весь кадр: x, y, u, v на вершину
    std::vector<float> m_scratch; // временный буфер под один вызов buildQuads
};