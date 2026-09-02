#include "Window.h"

#include <glad/glad.h>
#include <iostream>

Window::Window(const char* title, int width, int height)
    : m_width(width), m_height(height)
{
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << '\n';
        return;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    m_window = SDL_CreateWindow(title, width, height, SDL_WINDOW_OPENGL);

    if (!m_window)
    {
        std::cerr << "SDL_CreateWindow failed: " << SDL_GetError() << '\n';
        return;
    }

    m_glContext = SDL_GL_CreateContext(m_window);

    if (!m_glContext)
    {
        std::cerr << "SDL_GL_CreateContext failed: " << SDL_GetError() << '\n';
        SDL_DestroyWindow(m_window);
        m_window = nullptr;
        return;
    }

    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(SDL_GL_GetProcAddress)))
    {
        std::cerr << "Failed to load OpenGL functions.\n";
        SDL_GL_DestroyContext(m_glContext);
        SDL_DestroyWindow(m_window);
        m_glContext = nullptr;
        m_window = nullptr;
        return;
    }

    std::cout << "OpenGL functions loaded successfully.\n";

    // VSync off — важно для честных замеров производительности.
    if (!SDL_GL_SetSwapInterval(0))
    {
        std::cerr << "Warning: failed to disable VSync: " << SDL_GetError() << '\n';
    }
}

Window::~Window()
{
    if (m_glContext) SDL_GL_DestroyContext(m_glContext);
    if (m_window) SDL_DestroyWindow(m_window);
    SDL_Quit();
}

void Window::swap()
{
    SDL_GL_SwapWindow(m_window);
}

void Window::setTitle(const std::string& title)
{
    SDL_SetWindowTitle(m_window, title.c_str());
}