#include <glad/glad.h>
#include <SDL3/SDL.h>

#include "Input/InputState.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <sstream>
#include <vector>

const char* vertexShaderSource = R"(
#version 330 core

layout (location = 0) in vec2 aPos;

uniform float offsetX;
uniform float offsetY;

void main()
{
    gl_Position = vec4(
        aPos.x + offsetX,
        aPos.y + offsetY,
        0.0,
        1.0
    );
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
    // ------------------------------------------------------------
    // SDL
    // ------------------------------------------------------------

    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        std::cerr << "SDL_Init failed: "
            << SDL_GetError() << '\n';

        return 1;
    }

    // ------------------------------------------------------------
    // OpenGL
    // ------------------------------------------------------------

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);

    SDL_GL_SetAttribute(
        SDL_GL_CONTEXT_PROFILE_MASK,
        SDL_GL_CONTEXT_PROFILE_CORE
    );

    SDL_Window* window = SDL_CreateWindow(
        "AgarClient",
        1280,
        720,
        SDL_WINDOW_OPENGL
    );

    if (!window)
    {
        std::cerr << "SDL_CreateWindow failed: "
            << SDL_GetError() << '\n';

        SDL_Quit();
        return 1;
    }

    SDL_GLContext glContext = SDL_GL_CreateContext(window);

    if (!glContext)
    {
        std::cerr << "SDL_GL_CreateContext failed: "
            << SDL_GetError() << '\n';

        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }


    if (!gladLoadGLLoader(
        reinterpret_cast<GLADloadproc>(SDL_GL_GetProcAddress)))
    {
        std::cerr << "Failed to load OpenGL functions.\n";

        SDL_GL_DestroyContext(glContext);
        SDL_DestroyWindow(window);
        SDL_Quit();

        return 1;
    }

    std::cout << "OpenGL functions loaded successfully.\n";


    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);

    glShaderSource(
        vertexShader,
        1,
        &vertexShaderSource,
        nullptr
    );

    glCompileShader(vertexShader);

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);

    glShaderSource(
        fragmentShader,
        1,
        &fragmentShaderSource,
        nullptr
    );

    glCompileShader(fragmentShader);

    GLuint shaderProgram = glCreateProgram();

    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);

    glLinkProgram(shaderProgram);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    float vertices[] =
    {
        -0.05f, -0.05f,
         0.05f, -0.05f,
         0.05f,  0.05f,

        -0.05f, -0.05f,
         0.05f,  0.05f,
        -0.05f,  0.05f
    };

    GLuint VAO;
    GLuint VBO;

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    glBufferData(
        GL_ARRAY_BUFFER,
        sizeof(vertices),
        vertices,
        GL_STATIC_DRAW
    );

    glVertexAttribPointer(
        0,
        2,
        GL_FLOAT,
        GL_FALSE,
        2 * sizeof(float),
        nullptr
    );

    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // VSync OFF
    if (!SDL_GL_SetSwapInterval(0))
    {
        std::cerr << "Warning: failed to disable VSync: "
            << SDL_GetError() << '\n';
    }
    else
    {
        std::cout << "VSync disabled successfully.\n";
    }

    int swapInterval = 0;

    if (SDL_GL_GetSwapInterval(&swapInterval))
    {
        std::cout << "Current swap interval: "
            << swapInterval << '\n';
    }

    // ------------------------------------------------------------
    // Timing
    // ------------------------------------------------------------

    using Clock = std::chrono::steady_clock;

    auto lastFrame = Clock::now();
    auto statsStart = lastFrame;

    double frameTimeSum = 0.0;

    std::vector<double> frameTimes;
    frameTimes.reserve(10000);

    int frames = 0;
    int mouseEvents = 0;

    double fps = 0.0;
    double averageFrameTime = 0.0;
    double onePercentLow = 0.0;

    // ------------------------------------------------------------
    // Mouse
    // ------------------------------------------------------------

    InputState input;
    float objectX = 0.0f;
    float objectY = 0.0f;

    // ------------------------------------------------------------
    // Animation
    // ------------------------------------------------------------

    double animationTime = 0.0;

    bool running = true;

    // ------------------------------------------------------------
    // Main loop
    // ------------------------------------------------------------

    while (running)
    {
        auto frameStart = Clock::now();

        // --------------------------------------------------------
        // INPUT
        // --------------------------------------------------------

        SDL_Event event;

        while (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
            case SDL_EVENT_QUIT:
                running = false;
                break;

            case SDL_EVENT_MOUSE_MOTION:
                input.mouseX = event.motion.x;
                input.mouseY = event.motion.y;

                input.mouseDeltaX += event.motion.xrel;
                input.mouseDeltaY += event.motion.yrel;

                mouseEvents++;
                break;

            default:
                break;
            }
        }

        // --------------------------------------------------------
        // UPDATE
        // --------------------------------------------------------

        double deltaTime =
            std::chrono::duration<double>(
                frameStart - lastFrame
            ).count();

        lastFrame = frameStart;

        animationTime += deltaTime;

        objectX =
            (input.mouseX / 1280.0f) * 2.0f - 1.0f;

        objectY =
            1.0f - (input.mouseY / 720.0f) * 2.0f;

        // --------------------------------------------------------
        // RENDER
        // --------------------------------------------------------

        float r =
            static_cast<float>(
                (std::sin(animationTime) + 1.0) * 0.5
                );

        float g =
            static_cast<float>(
                (std::sin(animationTime * 1.7) + 1.0) * 0.5
                );

        float b =
            static_cast<float>(
                (std::sin(animationTime * 2.3) + 1.0) * 0.5
                );

        glClearColor(r, g, b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glUseProgram(shaderProgram);

        GLint offsetXLocation =
            glGetUniformLocation(shaderProgram, "offsetX");

        GLint offsetYLocation =
            glGetUniformLocation(shaderProgram, "offsetY");

        glUniform1f(offsetXLocation, objectX);
        glUniform1f(offsetYLocation, objectY);

        glBindVertexArray(VAO);

        glDrawArrays(
            GL_TRIANGLES,
            0,
            6
        );

        glBindVertexArray(0);

        SDL_GL_SwapWindow(window);

        // --------------------------------------------------------
        // FRAME TIME
        // --------------------------------------------------------

        auto frameEnd = Clock::now();

        double frameTime =
            std::chrono::duration<double, std::milli>(
                frameEnd - frameStart
            ).count();

        frameTimes.push_back(frameTime);

        frameTimeSum += frameTime;

        frames++;

        // --------------------------------------------------------
        // STATISTICS
        // --------------------------------------------------------

        double statsTime =
            std::chrono::duration<double>(
                frameEnd - statsStart
            ).count();

        if (statsTime >= 1.0)
        {
            fps = frames / statsTime;

            averageFrameTime =
                frameTimeSum / frames;

            // Sort frame times from worst to best.
            std::vector<double> sorted = frameTimes;

            std::sort(
                sorted.begin(),
                sorted.end(),
                std::greater<double>()
            );

            // 1% worst frames.
            size_t lowCount =
                std::max<size_t>(
                    1,
                    sorted.size() / 100
                );

            double worstOnePercent =
                std::accumulate(
                    sorted.begin(),
                    sorted.begin() + lowCount,
                    0.0
                ) / lowCount;

            // Convert worst 1% frame time into FPS.
            onePercentLow =
                1000.0 / worstOnePercent;

            // ----------------------------------------------------
            // Window title
            // ----------------------------------------------------

            std::ostringstream title;

            title
                << "AgarClient | "
                << std::fixed
                << std::setprecision(0)
                << fps
                << " FPS | avg "
                << std::setprecision(3)
                << averageFrameTime
                << " ms | 1% low "
                << std::setprecision(0)
                << onePercentLow
                << " FPS | mouse "
                << mouseEvents
                << " | pos "
                << std::setprecision(0)
                << input.mouseX
                << ", "
                << input.mouseY;

            SDL_SetWindowTitle(
                window,
                title.str().c_str()
            );

            // Save mouse event count before resetting.
            int mouseEventsThisSecond = mouseEvents;

            // Reset statistics.
            frames = 0;
            frameTimeSum = 0.0;
            frameTimes.clear();
            statsStart = frameEnd;
            mouseEvents = 0;

            // Console output.
            std::cout
                << "FPS: "
                << std::fixed
                << std::setprecision(0)
                << fps
                << " | Avg frame: "
                << std::setprecision(3)
                << averageFrameTime
                << " ms"
                << " | 1% low: "
                << std::setprecision(0)
                << onePercentLow
                << " FPS"
                << " | Mouse events: "
                << mouseEventsThisSecond
                << '\n';
        }
    }

    // ------------------------------------------------------------
    // Shutdown
    // ------------------------------------------------------------

    SDL_GL_DestroyContext(glContext);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}