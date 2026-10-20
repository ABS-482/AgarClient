#include "Window.h"

#include <glad/glad.h>

#include <chrono>
#include <cmath>
#include <iostream>

Window::Window(const char* title, int width, int height)
{
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << '\n';
        return;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);

    m_window = SDL_CreateWindow(title, width, height,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);

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

    glEnable(GL_MULTISAMPLE);

    setVSync(false);
    updateSize();          // m_width/m_height, glViewport, refresh rate
}

Window::~Window()
{
    if (m_glContext) SDL_GL_DestroyContext(m_glContext);
    if (m_window) SDL_DestroyWindow(m_window);
    SDL_Quit();
}

void Window::queryRefreshRate()
{
    const SDL_DisplayID display = SDL_GetDisplayForWindow(m_window);
    const SDL_DisplayMode* mode = SDL_GetCurrentDisplayMode(display);

    if (mode && mode->refresh_rate > 0.0f)
    {
        m_refreshRateExact = mode->refresh_rate;
        // lround, а не static_cast<int>: 143.998 Гц должно стать 144, а не 143
        m_refreshRate = static_cast<int>(std::lround(mode->refresh_rate));
    }
}

void Window::updateSize()
{
    if (!m_window) return;

    int w = 0, h = 0;
    if (!SDL_GetWindowSizeInPixels(m_window, &w, &h) || w <= 0 || h <= 0)
        return;

    if (w != m_width || h != m_height)
    {
        m_width = w;
        m_height = h;
        glViewport(0, 0, w, h);
        queryRefreshRate();
    }
}

bool Window::isFullscreen() const
{
    return m_fullscreen;
}

void Window::toggleFullscreen()
{
    if (!m_window)
        return;

    const bool isFullscreen =
        (SDL_GetWindowFlags(m_window) & SDL_WINDOW_FULLSCREEN) != 0;

    if (!isFullscreen)
    {
        // Координаты окна, а не пиксели (m_windowedWidth/Height должны быть int).
        SDL_GetWindowSize(m_window, &m_windowedWidth, &m_windowedHeight);

        // nullptr = borderless fullscreen в разрешении рабочего стола.
        SDL_SetWindowFullscreenMode(m_window, nullptr);

        if (!SDL_SetWindowFullscreen(m_window, true))
        {
            std::cerr << "SDL_SetWindowFullscreen(true) failed: "
                << SDL_GetError() << '\n';
            return;
        }
    }
    else
    {
        if (!SDL_SetWindowFullscreen(m_window, false))
        {
            std::cerr << "SDL_SetWindowFullscreen(false) failed: "
                << SDL_GetError() << '\n';
            return;
        }
    }

    // Best-effort: таймаут здесь не ошибка, состояние читаем из SDL ниже.
    SDL_SyncWindow(m_window);

    m_fullscreen =
        (SDL_GetWindowFlags(m_window) & SDL_WINDOW_FULLSCREEN) != 0;

    if (!m_fullscreen && m_windowedWidth > 0 && m_windowedHeight > 0)
        SDL_SetWindowSize(m_window, m_windowedWidth, m_windowedHeight);

    updateSize();
}

bool Window::setVSync(bool enabled)
{
    if (!SDL_GL_SetSwapInterval(enabled ? 1 : 0))
    {
        std::cerr << "Warning: failed to set swap interval: " << SDL_GetError() << '\n';
        return false;
    }

    m_vsync = enabled;
    return true;
}

void Window::swap()
{
    const auto t0 = std::chrono::steady_clock::now();

    SDL_GL_SwapWindow(m_window);

    if (m_syncAfterSwap)
        glFinish();

    m_lastSwapMs = std::chrono::duration<double, std::milli>(
        std::chrono::steady_clock::now() - t0).count();
}

void Window::setTitle(const std::string& title)
{
    SDL_SetWindowTitle(m_window, title.c_str());
}