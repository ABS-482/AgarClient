#include "Shader.h"

#include <iostream>

namespace
{
    GLuint compile(GLenum type, const char* src)
    {
        GLuint shader = glCreateShader(type);
        glShaderSource(shader, 1, &src, nullptr);
        glCompileShader(shader);

        GLint success = 0;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

        if (!success)
        {
            char log[512];
            glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
            std::cerr << "Shader compile error: " << log << '\n';
        }

        return shader;
    }
}

Shader::Shader(const char* vertexSrc, const char* fragmentSrc)
{
    GLuint vs = compile(GL_VERTEX_SHADER, vertexSrc);
    GLuint fs = compile(GL_FRAGMENT_SHADER, fragmentSrc);

    m_program = glCreateProgram();
    glAttachShader(m_program, vs);
    glAttachShader(m_program, fs);
    glLinkProgram(m_program);

    glDeleteShader(vs);
    glDeleteShader(fs);
}

Shader::~Shader()
{
    if (m_program) glDeleteProgram(m_program);
}

void Shader::use() const
{
    glUseProgram(m_program);
}

void Shader::setFloat(GLint location, float value) const
{
    glUniform1f(location, value);
}

void Shader::setInt(GLint location, int value) const
{
    glUniform1i(location, value);
}

void Shader::setVec2(GLint location, float x, float y) const
{
    glUniform2f(location, x, y);
}

void Shader::setVec3(GLint location, float x, float y, float z) const
{
    glUniform3f(location, x, y, z);
}

GLint Shader::uniformLocation(const char* name) const
{
    return glGetUniformLocation(m_program, name);
}