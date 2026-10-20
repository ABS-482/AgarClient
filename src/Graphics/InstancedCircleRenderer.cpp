#include "InstancedCircleRenderer.h"

InstancedCircleRenderer::InstancedCircleRenderer()
{
    const float quad[] =
    {
        -1.0f, -1.0f,
         1.0f, -1.0f,
         1.0f,  1.0f,

        -1.0f, -1.0f,
         1.0f,  1.0f,
        -1.0f,  1.0f
    };

    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_quadVbo);
    glGenBuffers(1, &m_instanceVbo);

    glBindVertexArray(m_vao);

    // Геометрия одного круга: квадрат из двух треугольников.
    glBindBuffer(GL_ARRAY_BUFFER, m_quadVbo);
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

    // Данные экземпляра:
    // center.x, center.y, radius, r, g, b, depth.
    glBindBuffer(GL_ARRAY_BUFFER, m_instanceVbo);

    constexpr GLsizei stride = 7 * sizeof(float);

    // location = 1: центр круга (x, y).
    glVertexAttribPointer(
        1,
        2,
        GL_FLOAT,
        GL_FALSE,
        stride,
        reinterpret_cast<void*>(0)
    );
    glEnableVertexAttribArray(1);
    glVertexAttribDivisor(1, 1);

    // location = 2: радиус.
    glVertexAttribPointer(
        2,
        1,
        GL_FLOAT,
        GL_FALSE,
        stride,
        reinterpret_cast<void*>(2 * sizeof(float))
    );
    glEnableVertexAttribArray(2);
    glVertexAttribDivisor(2, 1);

    // location = 3: цвет (r, g, b).
    glVertexAttribPointer(
        3,
        3,
        GL_FLOAT,
        GL_FALSE,
        stride,
        reinterpret_cast<void*>(3 * sizeof(float))
    );
    glEnableVertexAttribArray(3);
    glVertexAttribDivisor(3, 1);

    // location = 4: индивидуальная глубина экземпляра.
    glVertexAttribPointer(
        4,
        1,
        GL_FLOAT,
        GL_FALSE,
        stride,
        reinterpret_cast<void*>(6 * sizeof(float))
    );
    glEnableVertexAttribArray(4);
    glVertexAttribDivisor(4, 1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

InstancedCircleRenderer::~InstancedCircleRenderer()
{
    glDeleteBuffers(1, &m_quadVbo);
    glDeleteBuffers(1, &m_instanceVbo);
    glDeleteVertexArrays(1, &m_vao);
}

void InstancedCircleRenderer::draw(
    const float* instanceData,
    size_t instanceCount
)
{
    if (instanceCount == 0 || instanceData == nullptr)
        return;

    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_instanceVbo);

    glBufferData(
        GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(
            instanceCount * 7 * sizeof(float)
            ),
        instanceData,
        GL_DYNAMIC_DRAW
    );

    glDrawArraysInstanced(
        GL_TRIANGLES,
        0,
        6,
        static_cast<GLsizei>(instanceCount)
    );

    glBindVertexArray(0);
}