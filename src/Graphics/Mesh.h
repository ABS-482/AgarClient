#pragma once

#include <cstddef>

class Mesh
{
public:
    Mesh(const float* vertices, std::size_t vertexCount);
    ~Mesh();

    void draw() const;

    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;

private:
    unsigned int m_vao = 0;
    unsigned int m_vbo = 0;
    std::size_t m_vertexCount = 0;
};