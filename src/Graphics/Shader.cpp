#include "Shader.h"

#include <stdexcept>
#include <string>
#include <vector>

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
            GLint len = 0;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &len);

            std::vector<char> log(len > 1 ? len : 1, '\0');
            glGetShaderInfoLog(shader, static_cast<GLsizei>(log.size()), nullptr, log.data());
            glDeleteShader(shader);

            throw std::runtime_error(
                std::string(type == GL_VERTEX_SHADER ? "Vertex" : "Fragment") +
                " shader compile error:\n" + log.data()
            );
        }

        return shader;
    }
}

Shader::Shader(const char* vertexSrc, const char* fragmentSrc)
{
    GLuint vs = compile(GL_VERTEX_SHADER, vertexSrc);

    GLuint fs = 0;
    try
    {
        fs = compile(GL_FRAGMENT_SHADER, fragmentSrc);
    }
    catch (...)
    {
        glDeleteShader(vs);   // не оставляем утечку, если упал второй шейдер
        throw;
    }

    m_program = glCreateProgram();
    glAttachShader(m_program, vs);
    glAttachShader(m_program, fs);
    glLinkProgram(m_program);

    glDeleteShader(vs);
    glDeleteShader(fs);

    GLint linked = 0;
    glGetProgramiv(m_program, GL_LINK_STATUS, &linked);

    if (!linked)
    {
        GLint len = 0;
        glGetProgramiv(m_program, GL_INFO_LOG_LENGTH, &len);

        std::vector<char> log(len > 1 ? len : 1, '\0');
        glGetProgramInfoLog(m_program, static_cast<GLsizei>(log.size()), nullptr, log.data());

        glDeleteProgram(m_program);
        m_program = 0;

        throw std::runtime_error(std::string("Shader link error:\n") + log.data());
    }
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

void Shader::setVec4(GLint location, float x, float y, float z, float w) const
{
    glUniform4f(location, x, y, z, w);
}

GLint Shader::uniformLocation(const char* name) const
{
    return glGetUniformLocation(m_program, name);
}