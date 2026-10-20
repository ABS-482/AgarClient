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

    // Размер в пикселях (то, что нужно для glViewport).
    int width() const { return m_width; }
    int height() const { return m_height; }

    int   refreshRate() const { return m_refreshRate; }
    float refreshRateExact() const { return m_refreshRateExact; }

    // Дёшево, можно звать каждый кадр в начале: подхватывает смену размера
    // (resize / fullscreen), обновляет glViewport и частоту монитора.
    void updateSize();

    // Borderless fullscreen на текущем мониторе (в Windows даёт возможность
    // обойти композицию DWM -> меньше лага, чем в оконном режиме).
    void toggleFullscreen();
    bool isFullscreen() const;

    bool setVSync(bool enabled);
    bool vsync() const { return m_vsync; }

    // glFinish после swap: CPU не убегает вперёд, драйвер не копит очередь кадров.
    void setSyncAfterSwap(bool on) { m_syncAfterSwap = on; }
    bool syncAfterSwap() const { return m_syncAfterSwap; }

    void swap();

    // Сколько заняли SwapBuffers (+ glFinish). Стабильно > ~1 мс => очередь кадров / GPU-bound.
    double lastSwapMs() const { return m_lastSwapMs; }

    void setTitle(const std::string& title);

private:
    void queryRefreshRate();

    SDL_Window* m_window = nullptr;
    SDL_GLContext m_glContext = nullptr;

    int   m_width = 0;
    int   m_height = 0;
    int   m_refreshRate = 60;
    float m_refreshRateExact = 60.0f;

    bool   m_vsync = false;
    bool   m_syncAfterSwap = true;
    double m_lastSwapMs = 0.0;
    bool m_fullscreen = false;

    int m_windowedWidth = 1280;
    int m_windowedHeight = 720;
};