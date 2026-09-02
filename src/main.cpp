#include <glad/glad.h>
#include <SDL3/SDL.h>

#include "Core/FrameStats.h"
#include "Graphics/Shader.h"
#include "Graphics/Window.h"
#include "Graphics/Mesh.h"
#include "Graphics/Shaders/BasicShader.h"
#include "Input/InputManager.h"
#include "Input/InputState.h"
#include "Network/NetworkClient.h"
#include "Network/PacketHandler.h"
#include "Game/World.h"

#include <ixwebsocket/IXNetSystem.h>

#include <cmath>
#include <iomanip>
#include <iostream>
#include <sstream>

int main()
{
    ix::initNetSystem();

    Window window("AgarClient", 1280, 720);

    if (!window.isValid())
    {
        ix::uninitNetSystem();
        return 1;
    }

    Shader shader(BasicShader::vertex, BasicShader::fragment);

    GLint offsetXLocation = shader.uniformLocation("offsetX");
    GLint offsetYLocation = shader.uniformLocation("offsetY");

    float vertices[] =
    {
        -0.05f, -0.05f,
         0.05f, -0.05f,
         0.05f,  0.05f,

        -0.05f, -0.05f,
         0.05f,  0.05f,
        -0.05f,  0.05f
    };

    Mesh square(vertices, 6);

    World world;
    PacketHandler packetHandler(9, world);

    NetworkClient network(packetHandler);

    network.connect("wss://megasplit2.petridish.pw");
    network.setPlayerPassword("");

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
        shader.setFloat(offsetXLocation, objectX);
        shader.setFloat(offsetYLocation, objectY);

        square.draw();

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

    ix::uninitNetSystem();

    return 0;
}