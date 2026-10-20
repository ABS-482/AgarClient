// UiTextRenderer.h
#pragma once
#include "UiFont.h"
#include <vector>

class FrameProfiler;

class UiTextRenderer
{
public:
    UiTextRenderer();
    ~UiTextRenderer();

    void begin() { m_batch.clear(); }

    // x — левый край, y — ВЕРХ строки (как в CSS). Возвращает ширину.
    float add(
        const UiFont& font,
        const std::string& text,
        float x, float y,
        float r, float g, float b,
        float a = 1.0f,
        float scale = 1.0f
    );
    void beginProfile(FrameProfiler& profiler) noexcept;
    void flush(const UiFont& font, float screenW, float screenH);

private:
    GLuint m_vao = 0, m_vbo = 0, m_prog = 0;
    GLint m_uScreen = -1, m_uAtlas = -1, m_uGamma = -1;
    std::vector<float> m_batch, m_scratch;
    FrameProfiler* m_profileProfiler = nullptr;
    double m_profileBuildMilliseconds = 0.0;
};