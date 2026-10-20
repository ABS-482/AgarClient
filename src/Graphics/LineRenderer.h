#pragma once

#include <glad/glad.h>

class Camera;

class LineRenderer
{
public:
    explicit LineRenderer(Camera& camera);
    ~LineRenderer();

    LineRenderer(const LineRenderer&) = delete;
    LineRenderer& operator=(const LineRenderer&) = delete;

    void draw(
        float x1,
        float y1,
        float x2,
        float y2,
        float screenW,
        float screenH,
        float r,
        float g,
        float b,
        float width = 3.0f
    );

private:
    Camera& m_camera;

    GLuint m_vao = 0;
    GLuint m_vbo = 0;
    GLuint m_program = 0;

    GLint m_cameraPos = -1;
    GLint m_zoom = -1;
    GLint m_screenSize = -1;
    GLint m_viewportSize = -1;
    GLint m_color = -1;
};