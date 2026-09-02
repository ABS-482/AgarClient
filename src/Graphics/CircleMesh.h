#pragma once

#include <cstddef>

class CircleMesh
{
public:
    explicit CircleMesh(std::size_t segments = 64);
    ~CircleMesh();

    void draw() const;

    CircleMesh(const CircleMesh&) = delete;
    CircleMesh& operator=(const CircleMesh&) = delete;

private:
    unsigned int m_vao = 0;
    unsigned int m_vbo = 0;
    std::size_t m_vertexCount = 0;
};