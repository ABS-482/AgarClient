#include "UIPanel.h"

#include "Shaders/RoundedPanelShader.h"
#include "Shaders/RoundedCornersPanelShader.h"
#include "Shaders/BubbleTailShader.h"

UIPanel::UIPanel()
    : m_shader(
        RoundedPanelShader::vertex,
        RoundedPanelShader::fragment
    ),
    m_roundedCornersShader(
        RoundedCornersPanelShader::vertex,
        RoundedCornersPanelShader::fragment
    ),
    m_bubbleTailShader(
        BubbleTailShader::vertex,
        BubbleTailShader::fragment
    )
{
    // --------------------------------------------------------
    // RoundedPanelShader
    // --------------------------------------------------------

    m_uCenter =
        m_shader.uniformLocation("uCenter");

    m_uHalfSize =
        m_shader.uniformLocation("uHalfSize");

    m_uScreenSize =
        m_shader.uniformLocation("uScreenSize");

    m_uCornerRadius =
        m_shader.uniformLocation("uCornerRadius");

    m_uFillColor =
        m_shader.uniformLocation("uFillColor");

    m_uBorderColor =
        m_shader.uniformLocation("uBorderColor");

    m_uBorderWidth =
        m_shader.uniformLocation("uBorderWidth");

    // --------------------------------------------------------
    // RoundedCornersPanelShader
    // --------------------------------------------------------

    m_rcCenter =
        m_roundedCornersShader.uniformLocation("uCenter");

    m_rcHalfSize =
        m_roundedCornersShader.uniformLocation("uHalfSize");

    m_rcScreenSize =
        m_roundedCornersShader.uniformLocation("uScreenSize");

    m_rcCornerRadii =
        m_roundedCornersShader.uniformLocation("uCornerRadii");

    m_rcFillColor =
        m_roundedCornersShader.uniformLocation("uFillColor");

    m_rcBorderColor =
        m_roundedCornersShader.uniformLocation("uBorderColor");

    m_rcBorderWidth =
        m_roundedCornersShader.uniformLocation("uBorderWidth");

    // --------------------------------------------------------
    // BubbleTailShader
    // --------------------------------------------------------

    m_tailScreenSize =
        m_bubbleTailShader.uniformLocation("uScreenSize");

    m_tailColor =
        m_bubbleTailShader.uniformLocation("uColor");

    // --------------------------------------------------------
    // Общий полноэкранный quad для панелей
    // --------------------------------------------------------

    float quad[] =
    {
        -1.0f, -1.0f,
         1.0f, -1.0f,
         1.0f,  1.0f,

        -1.0f, -1.0f,
         1.0f,  1.0f,
        -1.0f,  1.0f
    };

    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);

    glBindVertexArray(m_vao);

    glBindBuffer(
        GL_ARRAY_BUFFER,
        m_vbo
    );

    glBufferData(
        GL_ARRAY_BUFFER,
        sizeof(quad),
        quad,
        GL_STATIC_DRAW
    );

    glVertexAttribPointer(
        0,
        2,
        GL_FLOAT,
        GL_FALSE,
        2 * sizeof(float),
        nullptr
    );

    glEnableVertexAttribArray(0);

    glBindBuffer(
        GL_ARRAY_BUFFER,
        0
    );

    glBindVertexArray(0);

    // --------------------------------------------------------
    // VAO/VBO для хвостика сообщения
    // Три вершины треугольника: 6 float
    // --------------------------------------------------------

    glGenVertexArrays(
        1,
        &m_tailVao
    );

    glGenBuffers(
        1,
        &m_tailVbo
    );

    glBindVertexArray(m_tailVao);

    glBindBuffer(
        GL_ARRAY_BUFFER,
        m_tailVbo
    );

    glBufferData(
        GL_ARRAY_BUFFER,
        6 * sizeof(float),
        nullptr,
        GL_DYNAMIC_DRAW
    );

    glVertexAttribPointer(
        0,
        2,
        GL_FLOAT,
        GL_FALSE,
        2 * sizeof(float),
        nullptr
    );

    glEnableVertexAttribArray(0);

    glBindBuffer(
        GL_ARRAY_BUFFER,
        0
    );

    glBindVertexArray(0);
}

UIPanel::~UIPanel()
{
    // Основная геометрия панелей
    glDeleteBuffers(
        1,
        &m_vbo
    );

    glDeleteVertexArrays(
        1,
        &m_vao
    );

    // Геометрия хвостика сообщения
    glDeleteBuffers(
        1,
        &m_tailVbo
    );

    glDeleteVertexArrays(
        1,
        &m_tailVao
    );
}

void UIPanel::draw(
    float centerX,
    float centerY,
    float width,
    float height,
    float cornerRadius,
    float fillR,
    float fillG,
    float fillB,
    float fillA,
    float borderR,
    float borderG,
    float borderB,
    float borderA,
    float borderWidth,
    float screenWidth,
    float screenHeight
)
{
    m_shader.use();

    m_shader.setVec2(
        m_uCenter,
        centerX,
        centerY
    );

    m_shader.setVec2(
        m_uHalfSize,
        width * 0.5f,
        height * 0.5f
    );

    m_shader.setVec2(
        m_uScreenSize,
        screenWidth,
        screenHeight
    );

    m_shader.setFloat(
        m_uCornerRadius,
        cornerRadius
    );

    glUniform4f(
        m_uFillColor,
        fillR,
        fillG,
        fillB,
        fillA
    );

    glUniform4f(
        m_uBorderColor,
        borderR,
        borderG,
        borderB,
        borderA
    );

    m_shader.setFloat(
        m_uBorderWidth,
        borderWidth
    );

    glBindVertexArray(m_vao);

    glDrawArrays(
        GL_TRIANGLES,
        0,
        6
    );

    glBindVertexArray(0);
}

void UIPanel::drawRoundedCorners(
    float centerX,
    float centerY,
    float width,
    float height,
    float topLeftRadius,
    float topRightRadius,
    float bottomRightRadius,
    float bottomLeftRadius,
    float fillR,
    float fillG,
    float fillB,
    float fillA,
    float borderR,
    float borderG,
    float borderB,
    float borderA,
    float borderWidth,
    float screenWidth,
    float screenHeight
)
{
    m_roundedCornersShader.use();

    m_roundedCornersShader.setVec2(
        m_rcCenter,
        centerX,
        centerY
    );

    m_roundedCornersShader.setVec2(
        m_rcHalfSize,
        width * 0.5f,
        height * 0.5f
    );

    m_roundedCornersShader.setVec2(
        m_rcScreenSize,
        screenWidth,
        screenHeight
    );

    m_roundedCornersShader.setVec4(
        m_rcCornerRadii,
        topLeftRadius,
        topRightRadius,
        bottomRightRadius,
        bottomLeftRadius
    );

    glUniform4f(
        m_rcFillColor,
        fillR,
        fillG,
        fillB,
        fillA
    );

    glUniform4f(
        m_rcBorderColor,
        borderR,
        borderG,
        borderB,
        borderA
    );

    m_roundedCornersShader.setFloat(
        m_rcBorderWidth,
        borderWidth
    );

    glBindVertexArray(m_vao);

    glDrawArrays(
        GL_TRIANGLES,
        0,
        6
    );

    glBindVertexArray(0);
}

void UIPanel::drawTriangle(
    float x0,
    float y0,
    float x1,
    float y1,
    float x2,
    float y2,
    float fillR,
    float fillG,
    float fillB,
    float fillA,
    float screenWidth,
    float screenHeight
)
{
    // --------------------------------------------------------
    // Три вершины треугольника в экранных координатах
    // --------------------------------------------------------

    const float vertices[] =
    {
        x0, y0,
        x1, y1,
        x2, y2
    };

    // --------------------------------------------------------
    // Shader
    // --------------------------------------------------------

    m_bubbleTailShader.use();

    m_bubbleTailShader.setVec2(
        m_tailScreenSize,
        screenWidth,
        screenHeight
    );

    glUniform4f(
        m_tailColor,
        fillR,
        fillG,
        fillB,
        fillA
    );

    // --------------------------------------------------------
    // Upload vertices and draw
    // --------------------------------------------------------

    glBindVertexArray(m_tailVao);

    glBindBuffer(
        GL_ARRAY_BUFFER,
        m_tailVbo
    );

    glBufferSubData(
        GL_ARRAY_BUFFER,
        0,
        sizeof(vertices),
        vertices
    );

    glDrawArrays(
        GL_TRIANGLES,
        0,
        3
    );

    glBindBuffer(
        GL_ARRAY_BUFFER,
        0
    );

    glBindVertexArray(0);
}