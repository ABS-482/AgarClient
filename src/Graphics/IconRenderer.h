#pragma once

#include "Shader.h"

#include <glad/glad.h>
#include <string>

class IconRenderer
{
public:
    IconRenderer();
    ~IconRenderer();

    IconRenderer(const IconRenderer&) = delete;
    IconRenderer& operator=(const IconRenderer&) = delete;

    // Синхронная загрузка PNG с диска — безопасно для локальных файлов,
    // вызывать один раз при старте. Возвращает 0 при неудаче.
    GLuint loadTexture(const std::string& path);

    void draw(
        GLuint texture,
        float centerX, float centerY,
        float width, float height,
        float screenWidth, float screenHeight
    );

private:
    Shader m_shader;

    GLint m_uCenter;
    GLint m_uHalfSize;
    GLint m_uScreenSize;
    GLint m_uIcon;

    GLuint m_vao = 0;
    GLuint m_vbo = 0;
};