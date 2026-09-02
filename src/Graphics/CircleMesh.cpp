#include "CircleMesh.h"

#include <glad/glad.h>

#include <cmath>
#include <vector>

CircleMesh::CircleMesh(std::size_t segments)
    : m_vertexCount(segments + 2)
{
    std::vector<float> vertices;
    vertices.reserve(m_vertexCount * 2);

    // Центр круга
    vertices.push_back(0.0f);
    vertices.push_back(0.0f);

    constexpr float pi = 3.14159265358979323846f;

    // Вершины по окружности
    for (std::size_t i = 0; i <= segments; ++i)
    {
        float angle =
            2.0f * pi * static_cast<float>(i) /
            static_cast<float>(segments);

        vertices.push_back(std::cos(angle));
        vertices.push_back(std::sin(angle));
    }

    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);

    glBindVertexArray(m_vao);

    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);

    glBufferData(
        GL_ARRAY_BUFFER,
        vertices.size() * sizeof(float),
        vertices.data(),
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

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

CircleMesh::~CircleMesh()
{
    glDeleteBuffers(1, &m_vbo);
    glDeleteVertexArrays(1, &m_vao);
}

void CircleMesh::draw() const
{
    glBindVertexArray(m_vao);

    glDrawArrays(
        GL_TRIANGLE_FAN,
        0,
        static_cast<GLsizei>(m_vertexCount)
    );

    glBindVertexArray(0);
}