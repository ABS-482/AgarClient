#pragma once

#include <glad/glad.h>

class Shader
{
public:
    Shader(const char* vertexSrc, const char* fragmentSrc);
    ~Shader();

    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;

    void use() const;
    void setFloat(GLint location, float value) const;
    void setInt(GLint location, int value) const;
    void setVec2(GLint location, float x, float y) const;
    void setVec3(GLint location, float x, float y, float z) const;
    GLint uniformLocation(const char* name) const;

    GLuint id() const { return m_program; }

private:
    GLuint m_program = 0;
};