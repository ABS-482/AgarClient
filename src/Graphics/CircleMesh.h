#pragma once

#include <glad/glad.h>

// Единичный квад (-1..1) — форма круга вычисляется в фрагментном
// шейдере через SDF, а не полигональной аппроксимацией.
class CircleMesh
{
public:
    CircleMesh();
    ~CircleMesh();

    CircleMesh(const CircleMesh&) = delete;
    CircleMesh& operator=(const CircleMesh&) = delete;

    void draw() const;

private:
    GLuint m_vao = 0;
    GLuint m_vbo = 0;
};