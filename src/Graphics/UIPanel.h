#pragma once

#include "Shader.h"

#include <glad/glad.h>

class UIPanel
{
public:
    UIPanel();
    ~UIPanel();

    UIPanel(const UIPanel&) = delete;
    UIPanel& operator=(const UIPanel&) = delete;

    void draw(
        float centerX, float centerY,
        float width, float height,
        float cornerRadius,
        float fillR, float fillG, float fillB, float fillA,
        float borderR, float borderG, float borderB, float borderA,
        float borderWidth,
        float screenWidth, float screenHeight
    );

private:
    Shader m_shader;

    GLint m_uCenter;
    GLint m_uHalfSize;
    GLint m_uScreenSize;
    GLint m_uCornerRadius;
    GLint m_uFillColor;
    GLint m_uBorderColor;
    GLint m_uBorderWidth;

    GLuint m_vao = 0;
    GLuint m_vbo = 0;
};