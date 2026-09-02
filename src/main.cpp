#include <glad/glad.h>
#include <SDL3/SDL.h>

#include "Core/FrameStats.h"
#include "Graphics/Shader.h"
#include "Graphics/Window.h"
#include "Input/InputManager.h"
#include "Input/InputState.h"

#include <cmath>
#include <iomanip>
#include <iostream>
#include <sstream>

const char* vertexShaderSource = R"(
#version 330 core

layout (location = 0) in vec2 aPos;

uniform float offsetX;
uniform float offsetY;

void main()
{
    gl_Position = vec4(aPos.x + offsetX, aPos.y + offsetY, 0.0, 1.0);
}
)";

const char* fragmentShaderSource = R"(
#version 330 core

out vec4 FragColor;

void main()
{
    FragColor = vec4(1.0, 1.0, 1.0, 1.0);
}
)";

int main()
{
    Window window("AgarClient", 1280, 720);

    if (!window.isValid())
        return 1;

    Shader shader(vertexShaderSource, fragmentShaderSource);

    float vertices[] =
    {
        -0.05f, -0.05f,
         0.05f, -0.05f,
         0.05f,  0.05f,

        -0.05f, -0.05f,
         0.05f,  0.05f,
        -0.05f,  0.05f
    };

    GLuint VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    InputManager inputManager;
    InputState input;
    FrameStats stats;

    double animationTime = 0.0;
    bool running = true;

    while (running)
    {
        stats.beginFrame();

        running = inputManager.poll(input);

        animationTime += stats.deltaTime();

        float objectX = (input.mouseX / 1280.0f) * 2.0f - 1.0f;
        float objectY = 1.0f - (input.mouseY / 720.0f) * 2.0f;

        float r = static_cast<float>((std::sin(animationTime) + 1.0) * 0.5);
        float g = static_cast<float>((std::sin(animationTime * 1.7) + 1.0) * 0.5);
        float b = static_cast<float>((std::sin(animationTime * 2.3) + 1.0) * 0.5);

        glClearColor(r, g, b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        shader.use();
        glUniform1f(shader.uniformLocation("offsetX"), objectX);
        glUniform1f(shader.uniformLocation("offsetY"), objectY);

        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);

        window.swap();

        stats.endFrame(inputManager.mouseEventsThisFrame());

        if (stats.hasNewStats())
        {
            std::ostringstream title;
            title << "AgarClient | "
                << std::fixed << std::setprecision(0) << stats.fps()
                << " FPS | avg " << std::setprecision(3) << stats.averageFrameTimeMs()
                << " ms | 1% low " << std::setprecision(0) << stats.onePercentLowFps()
                << " FPS | mouse " << stats.mouseEventsLastSecond()
                << " | pos " << std::setprecision(0) << input.mouseX << ", " << input.mouseY;

            window.setTitle(title.str());
        }
    }

    return 0;
}