#pragma once

#include <chrono>
#include <vector>

class FrameStats
{
public:
    using Clock = std::chrono::steady_clock;

    void beginFrame();
    void endFrame(int mouseEventsThisFrame);

    // true, если за этот вызов накопилась новая секунда статистики
    // (значит fps()/averageFrameTimeMs()/onePercentLowFps() свежие).
    bool hasNewStats() const { return m_hasNewStats; }

    double fps() const { return m_fps; }
    double averageFrameTimeMs() const { return m_averageFrameTimeMs; }
    double onePercentLowFps() const { return m_onePercentLowFps; }
    int mouseEventsLastSecond() const { return m_mouseEventsLastSecond; }
    double deltaTime() const { return m_deltaTime; }

private:
    Clock::time_point m_lastFrame = Clock::now();
    Clock::time_point m_statsStart = m_lastFrame;
    Clock::time_point m_frameStart;

    double m_frameTimeSum = 0.0;
    std::vector<double> m_frameTimes;

    int m_frames = 0;
    int m_mouseEvents = 0;
    int m_mouseEventsLastSecond = 0;

    double m_deltaTime = 0.0;
    double m_fps = 0.0;
    double m_averageFrameTimeMs = 0.0;
    double m_onePercentLowFps = 0.0;

    bool m_hasNewStats = false;
};