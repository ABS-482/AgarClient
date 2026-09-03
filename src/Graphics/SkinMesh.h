#pragma once

#include <glad/glad.h>

class SkinMesh
{
public:
    SkinMesh();
    ~SkinMesh();

    SkinMesh(const SkinMesh&) = delete;
    SkinMesh& operator=(const SkinMesh&) = delete;

    void draw() const;

private:
    GLuint m_vao = 0;
    GLuint m_vbo = 0;
};