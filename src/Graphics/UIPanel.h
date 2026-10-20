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

    void drawRoundedCorners(
        float centerX, float centerY,
        float width, float height,
        float topLeftRadius,
        float topRightRadius,
        float bottomRightRadius,
        float bottomLeftRadius,
        float fillR, float fillG, float fillB, float fillA,
        float borderR, float borderG, float borderB, float borderA,
        float borderWidth,
        float screenWidth, float screenHeight
    );

    void drawTriangle(
        float x0, float y0,
        float x1, float y1,
        float x2, float y2,
        float fillR, float fillG, float fillB, float fillA,
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

    Shader m_roundedCornersShader;

    GLint m_rcCenter;
    GLint m_rcHalfSize;
    GLint m_rcScreenSize;
    GLint m_rcCornerRadii;
    GLint m_rcFillColor;
    GLint m_rcBorderColor;
    GLint m_rcBorderWidth;
    Shader m_bubbleTailShader;

    GLint m_tailScreenSize = -1;
    GLint m_tailColor = -1;

    GLuint m_tailVao = 0;
    GLuint m_tailVbo = 0;
};