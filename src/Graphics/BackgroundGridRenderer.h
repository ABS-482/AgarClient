#pragma once

#include <glad/glad.h>

#include <vector>

class BackgroundGridRenderer
{
public:
    BackgroundGridRenderer() = default;
    ~BackgroundGridRenderer();

    BackgroundGridRenderer(const BackgroundGridRenderer&) = delete;
    BackgroundGridRenderer& operator=(const BackgroundGridRenderer&) = delete;

    // Рисует обычную светлую сетку Petridish на весь экран.
    // Вызывать после очистки экрана, но до отрисовки UI.
    void draw(float screenW, float screenH);

private:
    bool ensureInitialized();
    void release();

    GLuint m_program = 0;
    GLuint m_vao = 0;
    GLuint m_vbo = 0;
    GLint m_gridColorLocation = -1;
    bool m_initializationFailed = false;
    std::vector<float> m_vertices;
};
