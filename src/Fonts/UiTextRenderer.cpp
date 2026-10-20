// UiTextRenderer.cpp
#include "UiTextRenderer.h"
#include <chrono>
#include "../FrameProfiler.h"

static const char* kVS = R"(#version 330 core
layout(location=0) in vec2 aPos;
layout(location=1) in vec2 aUV;
layout(location=2) in vec4 aColor;
uniform vec2 uScreen;
out vec2 vUV;
out vec4 vColor;
void main()
{
    vec2 ndc = vec2(aPos.x / uScreen.x * 2.0 - 1.0,
                    1.0 - aPos.y / uScreen.y * 2.0);
    gl_Position = vec4(ndc, 0.0, 1.0);
    vUV = aUV;
    vColor = aColor;
})";

static const char* kFS = R"(#version 330 core
in vec2 vUV;
in vec4 vColor;
uniform sampler2D uAtlas;
uniform float uGamma;
out vec4 frag;

void main()
{
    float a = texture(uAtlas, vUV).r;
    a = pow(max(a, 0.0), uGamma);
    frag = vec4(vColor.rgb, vColor.a * a);
})";

static GLuint compile(GLenum type, const char* src)
{
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    return s;
}

UiTextRenderer::UiTextRenderer()
{
    GLuint vs = compile(GL_VERTEX_SHADER, kVS);
    GLuint fs = compile(GL_FRAGMENT_SHADER, kFS);
    m_prog = glCreateProgram();
    glAttachShader(m_prog, vs);
    glAttachShader(m_prog, fs);
    glLinkProgram(m_prog);
    glDeleteShader(vs);
    glDeleteShader(fs);

    m_uScreen = glGetUniformLocation(m_prog, "uScreen");
    m_uAtlas = glGetUniformLocation(m_prog, "uAtlas");
    m_uGamma = glGetUniformLocation(m_prog, "uGamma");

    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    const GLsizei stride = 8 * sizeof(float);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(2 * sizeof(float)));
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, stride, (void*)(4 * sizeof(float)));
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    glBindVertexArray(0);
}

UiTextRenderer::~UiTextRenderer()
{
    glDeleteBuffers(1, &m_vbo);
    glDeleteVertexArrays(1, &m_vao);
    glDeleteProgram(m_prog);
}

float UiTextRenderer::add(
    const UiFont& font,
    const std::string& text,
    float x,
    float y,
    float r,
    float g,
    float b,
    float a,
    float scale
)
{
    const bool profileBuild = m_profileProfiler != nullptr;

    const auto buildStart = profileBuild
        ? std::chrono::steady_clock::now()
        : std::chrono::steady_clock::time_point{};

    const auto finishBuildProfile = [&]()
        {
            if (profileBuild)
            {
                m_profileBuildMilliseconds +=
                    std::chrono::duration<double, std::milli>(
                        std::chrono::steady_clock::now() - buildStart
                    ).count();
            }
        };

    m_scratch.clear();

    // Не допускаем отрицательный масштаб.
    if (scale <= 0.0f || text.empty())
    {
        finishBuildProfile();
        return 0.0f;
    }

    // y — верх строки, baseline = верх + ascent.
    const float baselineY = y + font.ascent();

    const float width =
        font.buildQuads(
            text,
            x,
            baselineY,
            m_scratch
        );

    if (!m_scratch.empty())
    {
        constexpr std::size_t floatsPerVertex = 8;
        constexpr std::size_t floatsPerSourceVertex = 4;

        const std::size_t vertexCount =
            m_scratch.size() / floatsPerSourceVertex;

        const std::size_t batchOffset = m_batch.size();

        m_batch.resize(
            batchOffset + vertexCount * floatsPerVertex
        );

        float* destination = m_batch.data() + batchOffset;

        for (std::size_t i = 0;
            i + 3 < m_scratch.size();
            i += floatsPerSourceVertex)
        {
            // Координаты вершины с сохранением левого края строки.
            destination[0] =
                x + (m_scratch[i] - x) * scale;

            destination[1] =
                y + (m_scratch[i + 1] - y) * scale;

            // UV текстурного атласа.
            destination[2] = m_scratch[i + 2];
            destination[3] = m_scratch[i + 3];

            // Цвет вершины.
            destination[4] = r;
            destination[5] = g;
            destination[6] = b;
            destination[7] = a;

            destination += floatsPerVertex;
        }
    }

    finishBuildProfile();
    return width * scale;
}

void UiTextRenderer::flush(
    const UiFont& font,
    float screenW,
    float screenH)
{
    FrameProfiler* profiler = m_profileProfiler;

    const auto flushStart = profiler
        ? std::chrono::steady_clock::now()
        : std::chrono::steady_clock::time_point{};

    if (m_batch.empty())
    {
        if (profiler)
        {
            profiler->record(
                FrameProfiler::ChatTextBuild,
                m_profileBuildMilliseconds
            );

            profiler->record(
                FrameProfiler::ChatTextFlush,
                0.0
            );

            m_profileProfiler = nullptr;
            m_profileBuildMilliseconds = 0.0;
        }

        return;
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glUseProgram(m_prog);
    glUniform1f(m_uGamma, 0.55f);
    glUniform2f(m_uScreen, screenW, screenH);
    glUniform1i(m_uAtlas, 0);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, font.textureId());

    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);

    glBufferData(
        GL_ARRAY_BUFFER,
        m_batch.size() * sizeof(float),
        m_batch.data(),
        GL_DYNAMIC_DRAW
    );

    glDrawArrays(
        GL_TRIANGLES,
        0,
        static_cast<GLsizei>(m_batch.size() / 8)
    );

    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);

    m_batch.clear();

    if (profiler)
    {
        const double flushMilliseconds =
            std::chrono::duration<double, std::milli>(
                std::chrono::steady_clock::now() - flushStart
            ).count();

        profiler->record(
            FrameProfiler::ChatTextBuild,
            m_profileBuildMilliseconds
        );

        profiler->record(
            FrameProfiler::ChatTextFlush,
            flushMilliseconds
        );

        m_profileProfiler = nullptr;
        m_profileBuildMilliseconds = 0.0;
    }
}

void UiTextRenderer::beginProfile(FrameProfiler& profiler) noexcept
{
    m_profileProfiler = &profiler;
    m_profileBuildMilliseconds = 0.0;
}