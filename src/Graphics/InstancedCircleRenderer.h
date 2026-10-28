#pragma once

#include <glad/glad.h>

#include <cstddef>

class InstancedCircleRenderer
{
public:
    InstancedCircleRenderer();
    ~InstancedCircleRenderer();

    InstancedCircleRenderer(
        const InstancedCircleRenderer&
    ) = delete;

    InstancedCircleRenderer& operator=(
        const InstancedCircleRenderer&
        ) = delete;

    // Статические экземпляры: буфер обновляется только при изменении.
    void uploadStatic(
        const float* instanceData,
        size_t instanceCount
    );

    void drawStatic();

    // Динамические экземпляры: обновление каждый кадр.
    void draw(
        const float* instanceData,
        size_t instanceCount
    );

private:
    void setupInstanceAttribs(
        GLuint vao,
        GLuint instanceVbo
    );

    GLuint m_staticVao = 0;
    GLuint m_dynamicVao = 0;

    GLuint m_quadVbo = 0;
    GLuint m_staticInstanceVbo = 0;
    GLuint m_dynVbo = 0;

    size_t m_staticInstanceCount = 0;
    size_t m_dynCapacity = 0;
};