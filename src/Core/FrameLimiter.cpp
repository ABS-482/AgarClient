#include "FrameLimiter.h"

#include <thread>

FrameLimiter::FrameLimiter(int monitorRefreshRate)
    : m_monitorRefreshRate(monitorRefreshRate)
    , m_mode(FrameLimitMode::RefreshRate)
{
}

void FrameLimiter::beginFrame()
{
    m_frameStart = std::chrono::steady_clock::now();
}

void FrameLimiter::endFrame()
{
    int target = targetFps();

    if (target <= 0)
        return;

    double targetFrameTime = 1.0 / static_cast<double>(target);

    double elapsed = std::chrono::duration<double>(
        std::chrono::steady_clock::now() - m_frameStart
    ).count();

    if (elapsed < targetFrameTime)
    {
        std::this_thread::sleep_for(
            std::chrono::duration<double>(targetFrameTime - elapsed)
        );
    }
}

void FrameLimiter::cycleMode()
{
    switch (m_mode)
    {
    case FrameLimitMode::Unlimited:        m_mode = FrameLimitMode::DoubleRefreshRate; break;
    case FrameLimitMode::DoubleRefreshRate: m_mode = FrameLimitMode::RefreshRate;       break;
    case FrameLimitMode::RefreshRate:       m_mode = FrameLimitMode::Custom;            break;
    case FrameLimitMode::Custom:            m_mode = FrameLimitMode::Unlimited;         break;
    }
}

void FrameLimiter::setCustomFps(int fps)
{
    m_customFps = fps;
}

int FrameLimiter::targetFps() const
{
    switch (m_mode)
    {
    case FrameLimitMode::Unlimited:        return 0;
    case FrameLimitMode::DoubleRefreshRate: return m_monitorRefreshRate * 2;
    case FrameLimitMode::RefreshRate:       return m_monitorRefreshRate;
    case FrameLimitMode::Custom:            return m_customFps;
    }

    return 0;
}

const char* FrameLimiter::modeName() const
{
    switch (m_mode)
    {
    case FrameLimitMode::Unlimited:        return "Unlimited";
    case FrameLimitMode::DoubleRefreshRate: return "2x Refresh Rate";
    case FrameLimitMode::RefreshRate:       return "Refresh Rate";
    case FrameLimitMode::Custom:            return "Custom";
    }

    return "?";
}