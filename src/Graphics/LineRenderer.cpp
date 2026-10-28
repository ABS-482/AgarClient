#include "LineRenderer.h"

#include "Camera.h"

#include <iostream>

namespace
{
    const char* vertexShaderSource = R"(#version 330 core

        layout (location = 0) in vec2 aPosition;

        uniform vec2 uCameraPos;
        uniform float uZoom;
        uniform vec2 uScreenSize;
        uniform vec2 uViewportSize;

        void main()
        {
            float scaleX =
                uScreenSize.x / uViewportSize.x;

            float scaleY =
                uScreenSize.y / uViewportSize.y;

            float screenX =
                uScreenSize.x * 0.5 +
                (aPosition.x - uCameraPos.x) *
                scaleX /
                uZoom;

            float screenY =
                uScreenSize.y * 0.5 +
                (aPosition.y - uCameraPos.y) *
                scaleY /
                uZoom;

            float clipX =
                screenX /
                (uScreenSize.x * 0.5) -
                1.0;

            float clipY =
                1.0 -
                screenY /
                (uScreenSize.y * 0.5);

            gl_Position =
                vec4(
                    clipX,
                    clipY,
                    0.0,
                    1.0
                );
        }
    )";

    const char* fragmentShaderSource = R"(#version 330 core

        uniform vec3 uColor;

        out vec4 FragColor;

        void main()
        {
            FragColor =
                vec4(
                    uColor,
                    1.0
                );
        }
    )";

    GLuint compileShader(
        GLenum type,
        const char* source
    )
    {
        GLuint shader =
            glCreateShader(type);

        glShaderSource(
            shader,
            1,
            &source,
            nullptr
        );

        glCompileShader(shader);

        GLint success = 0;

        glGetShaderiv(
            shader,
            GL_COMPILE_STATUS,
            &success
        );

        if (!success)
        {
            char log[512];

            glGetShaderInfoLog(
                shader,
                sizeof(log),
                nullptr,
                log
            );

            std::cerr
                << "Line shader compile error: "
                << log
                << '\n';
        }

        return shader;
    }
}

LineRenderer::LineRenderer(Camera& camera)
    : m_camera(camera)
{
    const GLuint vertexShader =
        compileShader(
            GL_VERTEX_SHADER,
            vertexShaderSource
        );

    const GLuint fragmentShader =
        compileShader(
            GL_FRAGMENT_SHADER,
            fragmentShaderSource
        );

    m_program =
        glCreateProgram();

    glAttachShader(
        m_program,
        vertexShader
    );

    glAttachShader(
        m_program,
        fragmentShader
    );

    glLinkProgram(
        m_program
    );

    GLint success = 0;

    glGetProgramiv(
        m_program,
        GL_LINK_STATUS,
        &success
    );

    if (!success)
    {
        char log[512];

        glGetProgramInfoLog(
            m_program,
            sizeof(log),
            nullptr,
            log
        );

        std::cerr
            << "Line shader link error: "
            << log
            << '\n';
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    m_cameraPos =
        glGetUniformLocation(
            m_program,
            "uCameraPos"
        );

    m_zoom =
        glGetUniformLocation(
            m_program,
            "uZoom"
        );

    m_screenSize =
        glGetUniformLocation(
            m_program,
            "uScreenSize"
        );

    m_viewportSize =
        glGetUniformLocation(
            m_program,
            "uViewportSize"
        );

    m_color =
        glGetUniformLocation(
            m_program,
            "uColor"
        );

    glGenVertexArrays(
        1,
        &m_vao
    );

    glGenBuffers(
        1,
        &m_vbo
    );

    glBindVertexArray(
        m_vao
    );

    glBindBuffer(
        GL_ARRAY_BUFFER,
        m_vbo
    );

    glBufferData(
        GL_ARRAY_BUFFER,
        sizeof(float) * 4,
        nullptr,
        GL_DYNAMIC_DRAW
    );

    glVertexAttribPointer(
        0,
        2,
        GL_FLOAT,
        GL_FALSE,
        sizeof(float) * 2,
        nullptr
    );

    glEnableVertexAttribArray(0);

    glBindBuffer(
        GL_ARRAY_BUFFER,
        0
    );

    glBindVertexArray(0);
}

LineRenderer::~LineRenderer()
{
    if (m_vbo != 0)
        glDeleteBuffers(
            1,
            &m_vbo
        );

    if (m_vao != 0)
        glDeleteVertexArrays(
            1,
            &m_vao
        );

    if (m_program != 0)
        glDeleteProgram(
            m_program
        );
}

void LineRenderer::draw(
    float x1,
    float y1,
    float x2,
    float y2,
    float screenW,
    float screenH,
    float r,
    float g,
    float b,
    float width
)
{
    glUseProgram(
        m_program
    );

    glUniform2f(
        m_cameraPos,
        m_camera.pos.x,
        m_camera.pos.y
    );

    glUniform1f(
        m_zoom,
        m_camera.zoom
    );

    glUniform2f(
        m_screenSize,
        screenW,
        screenH
    );

    glUniform2f(
        m_viewportSize,
        m_camera.vw,
        m_camera.vh
    );

    glUniform3f(
        m_color,
        r,
        g,
        b
    );

    const float vertices[] =
    {
        x1, y1,
        x2, y2
    };

    glBindVertexArray(
        m_vao
    );

    glBindBuffer(
        GL_ARRAY_BUFFER,
        m_vbo
    );

    glBufferSubData(
        GL_ARRAY_BUFFER,
        0,
        sizeof(vertices),
        vertices
    );

    glLineWidth(width);

    glDrawArrays(
        GL_LINES,
        0,
        2
    );

    glBindBuffer(
        GL_ARRAY_BUFFER,
        0
    );

    glBindVertexArray(0);
}