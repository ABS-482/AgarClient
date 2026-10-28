#include "BackgroundGridRenderer.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <vector>

namespace
{
    // Java: Level.makeLines() использует шаг 50, а AbstractScreen
    // временно устанавливает camera.zoom = 0.9f. При экранных
    // координатах это соответствует приблизительно 50 / 0.9 пикселям.
    constexpr float kGridSpacing = 50.0f / 0.9f;

    // Java: GRID_COLOR = new Color(0xCDD3D6FF).
    // При zoom = 0.9: alpha = 1.0 - 0.9 / 12.0 = 0.925.
    constexpr float kGridRed = 205.0f / 255.0f;
    constexpr float kGridGreen = 211.0f / 255.0f;
    constexpr float kGridBlue = 214.0f / 255.0f;
    constexpr float kGridAlpha = 0.925f;

    GLuint compileShader(GLenum type, const char* source)
    {
        const GLuint shader = glCreateShader(type);
        if (shader == 0)
        {
            std::cerr << "BackgroundGridRenderer: glCreateShader failed.\n";
            return 0;
        }

        glShaderSource(shader, 1, &source, nullptr);
        glCompileShader(shader);

        GLint compiled = GL_FALSE;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
        if (compiled == GL_TRUE)
            return shader;

        GLint logLength = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);

        std::vector<char> log(
            static_cast<std::size_t>(std::max(logLength, 1)), '\0');
        glGetShaderInfoLog(shader, logLength, nullptr, log.data());

        std::cerr << "BackgroundGridRenderer shader compilation failed: "
                  << log.data() << '\n';

        glDeleteShader(shader);
        return 0;
    }

    void appendVertex(
        std::vector<float>& vertices,
        float x,
        float y,
        float screenW,
        float screenH)
    {
        // Преобразование экранных координат (начало в левом верхнем углу)
        // в OpenGL NDC [-1, 1].
        const float ndcX = (2.0f * x / screenW) - 1.0f;
        const float ndcY = 1.0f - (2.0f * y / screenH);

        vertices.push_back(ndcX);
        vertices.push_back(ndcY);
    }
}

BackgroundGridRenderer::~BackgroundGridRenderer()
{
    release();
}

bool BackgroundGridRenderer::ensureInitialized()
{
    if (m_program != 0 && m_vao != 0 && m_vbo != 0)
        return true;

    if (m_initializationFailed)
        return false;

    // Ленивая инициализация: OpenGL-контекст должен быть активен к моменту
    // первого draw(), но не обязательно в момент создания MainMenu.
    static constexpr char vertexShaderSource[] = R"GLSL(
        #version 330 core
        layout (location = 0) in vec2 aPosition;
        void main()
        {
            gl_Position = vec4(aPosition, 0.0, 1.0);
        }
    )GLSL";

    static constexpr char fragmentShaderSource[] = R"GLSL(
        #version 330 core
        uniform vec4 uGridColor;
        out vec4 FragColor;
        void main()
        {
            FragColor = uGridColor;
        }
    )GLSL";

    const GLuint vertexShader =
        compileShader(GL_VERTEX_SHADER, vertexShaderSource);
    if (vertexShader == 0)
    {
        m_initializationFailed = true;
        return false;
    }

    const GLuint fragmentShader =
        compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);
    if (fragmentShader == 0)
    {
        glDeleteShader(vertexShader);
        m_initializationFailed = true;
        return false;
    }

    m_program = glCreateProgram();
    if (m_program == 0)
    {
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        m_initializationFailed = true;
        std::cerr << "BackgroundGridRenderer: glCreateProgram failed.\n";
        return false;
    }

    glAttachShader(m_program, vertexShader);
    glAttachShader(m_program, fragmentShader);
    glLinkProgram(m_program);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    GLint linked = GL_FALSE;
    glGetProgramiv(m_program, GL_LINK_STATUS, &linked);
    if (linked != GL_TRUE)
    {
        GLint logLength = 0;
        glGetProgramiv(m_program, GL_INFO_LOG_LENGTH, &logLength);

        std::vector<char> log(
            static_cast<std::size_t>(std::max(logLength, 1)), '\0');
        glGetProgramInfoLog(m_program, logLength, nullptr, log.data());

        std::cerr << "BackgroundGridRenderer program linking failed: "
                  << log.data() << '\n';

        glDeleteProgram(m_program);
        m_program = 0;
        m_initializationFailed = true;
        return false;
    }

    m_gridColorLocation = glGetUniformLocation(m_program, "uGridColor");

    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);

    if (m_vao == 0 || m_vbo == 0)
    {
        release();
        m_initializationFailed = true;
        std::cerr << "BackgroundGridRenderer: failed to create VAO/VBO.\n";
        return false;
    }

    GLint previousVao = 0;
    GLint previousArrayBuffer = 0;
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &previousVao);
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &previousArrayBuffer);

    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(
        0, 2, GL_FLOAT, GL_FALSE,
        static_cast<GLsizei>(2 * sizeof(float)), nullptr);

    glBindVertexArray(static_cast<GLuint>(previousVao));
    glBindBuffer(GL_ARRAY_BUFFER, static_cast<GLuint>(previousArrayBuffer));

    return true;
}

void BackgroundGridRenderer::draw(float screenW, float screenH)
{
    if (screenW <= 0.0f || screenH <= 0.0f || !ensureInitialized())
        return;

    m_vertices.clear();

    const std::size_t estimatedFloatCount =
        static_cast<std::size_t>(
            ((screenW + screenH) / kGridSpacing + 4.0f) * 4.0f);

    if (m_vertices.capacity() < estimatedFloatCount)
        m_vertices.reserve(estimatedFloatCount);

    const auto appendLine = [&](float x1, float y1, float x2, float y2)
    {
        appendVertex(m_vertices, x1, y1, screenW, screenH);
        appendVertex(m_vertices, x2, y2, screenW, screenH);
    };

    // Привязываем сетку к центру экрана: линии остаются на равном
    // расстоянии и при изменении размеров окна.
    float firstX = std::fmod(screenW * 0.5f, kGridSpacing);
    if (firstX < 0.0f)
        firstX += kGridSpacing;

    for (float x = firstX; x <= screenW; x += kGridSpacing)
        appendLine(x, 0.0f, x, screenH);

    float firstY = std::fmod(screenH * 0.5f, kGridSpacing);
    if (firstY < 0.0f)
        firstY += kGridSpacing;

    for (float y = firstY; y <= screenH; y += kGridSpacing)
        appendLine(0.0f, y, screenW, y);

    if (m_vertices.empty())
        return;

    // Сохраняем изменяемое состояние OpenGL, чтобы сетка не влияла
    // на следующие отрисовщики интерфейса.
    GLint previousProgram = 0;
    GLint previousVao = 0;
    GLint previousArrayBuffer = 0;
    GLint previousBlendSrcRgb = 0;
    GLint previousBlendDstRgb = 0;
    GLint previousBlendSrcAlpha = 0;
    GLint previousBlendDstAlpha = 0;
    GLfloat previousLineWidth = 1.0f;

    glGetIntegerv(GL_CURRENT_PROGRAM, &previousProgram);
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &previousVao);
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &previousArrayBuffer);
    glGetIntegerv(GL_BLEND_SRC_RGB, &previousBlendSrcRgb);
    glGetIntegerv(GL_BLEND_DST_RGB, &previousBlendDstRgb);
    glGetIntegerv(GL_BLEND_SRC_ALPHA, &previousBlendSrcAlpha);
    glGetIntegerv(GL_BLEND_DST_ALPHA, &previousBlendDstAlpha);
    glGetFloatv(GL_LINE_WIDTH, &previousLineWidth);

    const GLboolean blendWasEnabled = glIsEnabled(GL_BLEND);
    const GLboolean depthWasEnabled = glIsEnabled(GL_DEPTH_TEST);
    const GLboolean scissorWasEnabled = glIsEnabled(GL_SCISSOR_TEST);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_SCISSOR_TEST);
    glLineWidth(1.0f);

    glUseProgram(m_program);
    glUniform4f(
        m_gridColorLocation,
        kGridRed,
        kGridGreen,
        kGridBlue,
        kGridAlpha);

    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(
        GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(m_vertices.size() * sizeof(float)),
        m_vertices.data(),
        GL_DYNAMIC_DRAW);

    glDrawArrays(
        GL_LINES,
        0,
        static_cast<GLsizei>(m_vertices.size() / 2));

    glBindVertexArray(static_cast<GLuint>(previousVao));
    glBindBuffer(GL_ARRAY_BUFFER, static_cast<GLuint>(previousArrayBuffer));
    glUseProgram(static_cast<GLuint>(previousProgram));

    glBlendFuncSeparate(
        static_cast<GLenum>(previousBlendSrcRgb),
        static_cast<GLenum>(previousBlendDstRgb),
        static_cast<GLenum>(previousBlendSrcAlpha),
        static_cast<GLenum>(previousBlendDstAlpha));
    glLineWidth(previousLineWidth);

    if (blendWasEnabled)
        glEnable(GL_BLEND);
    else
        glDisable(GL_BLEND);

    if (depthWasEnabled)
        glEnable(GL_DEPTH_TEST);
    else
        glDisable(GL_DEPTH_TEST);

    if (scissorWasEnabled)
        glEnable(GL_SCISSOR_TEST);
    else
        glDisable(GL_SCISSOR_TEST);
}

void BackgroundGridRenderer::release()
{
    if (m_vbo != 0)
    {
        glDeleteBuffers(1, &m_vbo);
        m_vbo = 0;
    }

    if (m_vao != 0)
    {
        glDeleteVertexArrays(1, &m_vao);
        m_vao = 0;
    }

    if (m_program != 0)
    {
        glDeleteProgram(m_program);
        m_program = 0;
    }
}
