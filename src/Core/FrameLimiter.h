#pragma once
#include <chrono>

class FrameLimiter
{
public:
    FrameLimiter();
    ~FrameLimiter();

    FrameLimiter(const FrameLimiter&) = delete;
    FrameLimiter& operator=(const FrameLimiter&) = delete;

    void setTargetFps(int fps);                 // 0 = без лимита
    int  targetFps() const { return m_fps; }

    // Вызывать в самом конце кадра (после swap).
    void endFrame();

    // Диагностика (для оверлея / профайлера)
    double lastWaitMs() const { return m_lastWaitMs; }
    double spinMarginMs() const
    {
        return std::chrono::duration<double, std::milli>(m_margin).count();
    }

private:
    using Clock = std::chrono::steady_clock;
    using Ns = std::chrono::nanoseconds;

    void sleepUntil(Clock::time_point t);

    void* m_timer = nullptr;      // HANDLE (high-res waitable timer)
    void* m_mmcss = nullptr;      // HANDLE (MMCSS task)
    bool  m_periodSet = false;    // timeBeginPeriod(1) вызван

    int m_fps = 360;
    Ns  m_frame{ 1'000'000'000 / 360 };
    Ns  m_margin{ 1'000'000 };    // адаптивный запас на busy-wait, старт 1 мс

    Clock::time_point m_next{};
    bool   m_started = false;
    double m_lastWaitMs = 0.0;
};