#include "InstancedCircleRenderer.h"

#include <algorithm>
#include <cstdint>

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

    glGenVertexArrays(1, &m_staticVao);
    glGenVertexArrays(1, &m_dynamicVao);

    glGenBuffers(1, &m_quadVbo);
    glGenBuffers(1, &m_staticInstanceVbo);
    glGenBuffers(1, &m_dynVbo);

    // Общая геометрия квадрата для обоих VAO.
    glBindBuffer(GL_ARRAY_BUFFER, m_quadVbo);
    glBufferData(
        GL_ARRAY_BUFFER,
        sizeof(quad),
        quad,
        GL_STATIC_DRAW
    );

    // Каждый VAO запоминает свой instance VBO.
    setupInstanceAttribs(
        m_staticVao,
        m_staticInstanceVbo
    );

    setupInstanceAttribs(
        m_dynamicVao,
        m_dynVbo
    );

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void InstancedCircleRenderer::setupInstanceAttribs(
    GLuint vao,
    GLuint instanceVbo
)
{
    glBindVertexArray(vao);

    // location 0: координаты вершин квадрата.
    glBindBuffer(GL_ARRAY_BUFFER, m_quadVbo);

    glVertexAttribPointer(
        0,
        2,
        GL_FLOAT,
        GL_FALSE,
        2 * sizeof(float),
        nullptr
    );
    glEnableVertexAttribArray(0);

    // Формат экземпляра:
    // center.x, center.y, radius, r, g, b, depth.
    constexpr GLsizei stride = 7 * sizeof(float);

    glBindBuffer(GL_ARRAY_BUFFER, instanceVbo);

    // location 1: центр (x, y).
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

    // location 2: радиус.
    glVertexAttribPointer(
        2,
        1,
        GL_FLOAT,
        GL_FALSE,
        stride,
        reinterpret_cast<void*>(
            2 * sizeof(float)
            )
    );
    glEnableVertexAttribArray(2);
    glVertexAttribDivisor(2, 1);

    // location 3: цвет (r, g, b).
    glVertexAttribPointer(
        3,
        3,
        GL_FLOAT,
        GL_FALSE,
        stride,
        reinterpret_cast<void*>(
            3 * sizeof(float)
            )
    );
    glEnableVertexAttribArray(3);
    glVertexAttribDivisor(3, 1);

    // location 4: глубина.
    glVertexAttribPointer(
        4,
        1,
        GL_FLOAT,
        GL_FALSE,
        stride,
        reinterpret_cast<void*>(
            6 * sizeof(float)
            )
    );
    glEnableVertexAttribArray(4);
    glVertexAttribDivisor(4, 1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

InstancedCircleRenderer::~InstancedCircleRenderer()
{
    glDeleteBuffers(1, &m_quadVbo);
    glDeleteBuffers(1, &m_staticInstanceVbo);
    glDeleteBuffers(1, &m_dynVbo);

    glDeleteVertexArrays(1, &m_staticVao);
    glDeleteVertexArrays(1, &m_dynamicVao);
}

void InstancedCircleRenderer::uploadStatic(
    const float* instanceData,
    size_t instanceCount
)
{
    m_staticInstanceCount = 0;

    if (instanceCount == 0 || instanceData == nullptr)
        return;

    const size_t bytes =
        instanceCount * 7 * sizeof(float);

    glBindBuffer(
        GL_ARRAY_BUFFER,
        m_staticInstanceVbo
    );

    glBufferData(
        GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(bytes),
        instanceData,
        GL_STATIC_DRAW
    );

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    m_staticInstanceCount = instanceCount;
}

void InstancedCircleRenderer::drawStatic()
{
    if (m_staticInstanceCount == 0)
        return;

    glBindVertexArray(m_staticVao);

    glDrawArraysInstanced(
        GL_TRIANGLES,
        0,
        6,
        static_cast<GLsizei>(m_staticInstanceCount)
    );

    glBindVertexArray(0);
}

void InstancedCircleRenderer::draw(
    const float* instanceData,
    size_t instanceCount
)
{
    if (instanceCount == 0 || instanceData == nullptr)
        return;

    const size_t bytes =
        instanceCount * 7 * sizeof(float);

    glBindVertexArray(m_dynamicVao);
    glBindBuffer(GL_ARRAY_BUFFER, m_dynVbo);

    // Перевыделяем память только при нехватке ёмкости.
    if (bytes > m_dynCapacity)
    {
        m_dynCapacity = std::max(
            bytes * 2,
            static_cast<size_t>(7 * sizeof(float))
        );

        glBufferData(
            GL_ARRAY_BUFFER,
            static_cast<GLsizeiptr>(m_dynCapacity),
            nullptr,
            GL_STREAM_DRAW
        );
    }

    // Обновляем начало динамического буфера.
    glBufferSubData(
        GL_ARRAY_BUFFER,
        0,
        static_cast<GLsizeiptr>(bytes),
        instanceData
    );

    glDrawArraysInstanced(
        GL_TRIANGLES,
        0,
        6,
        static_cast<GLsizei>(instanceCount)
    );

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}