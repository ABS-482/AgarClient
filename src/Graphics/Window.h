#pragma once

#include <string>
#include <SDL3/SDL.h>

class Window
{
public:
    Window(const char* title, int width, int height);
    ~Window();

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    bool isValid() const { return m_window != nullptr && m_glContext != nullptr; }

    SDL_Window* handle() const { return m_window; }
    int width() const { return m_width; }
    int height() const { return m_height; }

    void swap();
    void setTitle(const std::string& title);

private:
    SDL_Window* m_window = nullptr;
    SDL_GLContext m_glContext = nullptr;
    int m_width = 0;
    int m_height = 0;
};