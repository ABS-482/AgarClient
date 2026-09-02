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
    GLint uniformLocation(const char* name) const;

    GLuint id() const { return m_program; }

private:
    GLuint m_program = 0;
};