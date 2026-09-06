#pragma once

#include <chrono>

enum class FrameLimitMode
{
    Unlimited,
    DoubleRefreshRate,
    RefreshRate,
    Custom
};

class FrameLimiter
{
public:
    explicit FrameLimiter(int monitorRefreshRate);

    void beginFrame();
    void endFrame();

    void cycleMode();
    void setCustomFps(int fps);

    FrameLimitMode mode() const { return m_mode; }
    int targetFps() const; // 0 = без ограничения
    const char* modeName() const;

private:
    int m_monitorRefreshRate;
    FrameLimitMode m_mode;
    int m_customFps = 360; // как у вас в Java-клиенте

    std::chrono::steady_clock::time_point m_frameStart;
};