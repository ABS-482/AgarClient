#pragma once

#include <glad/glad.h>

#include <cstddef>

class InstancedCircleRenderer
{
public:
    InstancedCircleRenderer();
    ~InstancedCircleRenderer();

    InstancedCircleRenderer(const InstancedCircleRenderer&) = delete;
    InstancedCircleRenderer& operator=(const InstancedCircleRenderer&) = delete;

    // instanceData: подряд center.x, center.y, radius, r, g, b — 6 float на инстанс.
    void draw(const float* instanceData, size_t instanceCount);

private:
    GLuint m_vao = 0;
    GLuint m_quadVbo = 0;
    GLuint m_instanceVbo = 0;
};