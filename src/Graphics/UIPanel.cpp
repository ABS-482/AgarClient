#include "UIPanel.h"

#include "Shaders/RoundedPanelShader.h"

UIPanel::UIPanel()
    : m_shader(RoundedPanelShader::vertex, RoundedPanelShader::fragment)
{
    m_uCenter = m_shader.uniformLocation("uCenter");
    m_uHalfSize = m_shader.uniformLocation("uHalfSize");
    m_uScreenSize = m_shader.uniformLocation("uScreenSize");
    m_uCornerRadius = m_shader.uniformLocation("uCornerRadius");
    m_uFillColor = m_shader.uniformLocation("uFillColor");
    m_uBorderColor = m_shader.uniformLocation("uBorderColor");
    m_uBorderWidth = m_shader.uniformLocation("uBorderWidth");

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
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

UIPanel::~UIPanel()
{
    glDeleteBuffers(1, &m_vbo);
    glDeleteVertexArrays(1, &m_vao);
}

void UIPanel::draw(
    float centerX, float centerY,
    float width, float height,
    float cornerRadius,
    float fillR, float fillG, float fillB, float fillA,
    float borderR, float borderG, float borderB, float borderA,
    float borderWidth,
    float screenWidth, float screenHeight
)
{
    m_shader.use();

    m_shader.setVec2(m_uCenter, centerX, centerY);
    m_shader.setVec2(m_uHalfSize, width * 0.5f, height * 0.5f);
    m_shader.setVec2(m_uScreenSize, screenWidth, screenHeight);
    m_shader.setFloat(m_uCornerRadius, cornerRadius);

    glUniform4f(m_uFillColor, fillR, fillG, fillB, fillA);
    glUniform4f(m_uBorderColor, borderR, borderG, borderB, borderA);

    m_shader.setFloat(m_uBorderWidth, borderWidth);

    glBindVertexArray(m_vao);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}