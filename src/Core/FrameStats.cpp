#include "FrameStats.h"

#include <algorithm>
#include <numeric>

void FrameStats::beginFrame()
{
    m_frameStart = Clock::now();
    m_deltaTime = std::chrono::duration<double>(m_frameStart - m_lastFrame).count();
    m_lastFrame = m_frameStart;
    m_hasNewStats = false;

    if (m_frameTimes.empty())
        m_frameTimes.reserve(10000);
}

void FrameStats::endFrame(int mouseEventsThisFrame)
{
    m_mouseEvents += mouseEventsThisFrame;

    auto frameEnd = Clock::now();
    double frameTimeMs = std::chrono::duration<double, std::milli>(frameEnd - m_frameStart).count();

    m_frameTimes.push_back(frameTimeMs);
    m_frameTimeSum += frameTimeMs;
    m_frames++;

    double statsTime = std::chrono::duration<double>(frameEnd - m_statsStart).count();

    if (statsTime >= 1.0)
    {
        m_fps = m_frames / statsTime;
        m_averageFrameTimeMs = m_frameTimeSum / m_frames;

        std::vector<double> sorted = m_frameTimes;
        std::sort(sorted.begin(), sorted.end(), std::greater<double>());

        size_t lowCount = std::max<size_t>(1, sorted.size() / 100);
        double worstOnePercent =
            std::accumulate(sorted.begin(), sorted.begin() + lowCount, 0.0) / lowCount;

        m_onePercentLowFps = 1000.0 / worstOnePercent;
        m_mouseEventsLastSecond = m_mouseEvents;

        m_frames = 0;
        m_frameTimeSum = 0.0;
        m_frameTimes.clear();
        m_statsStart = frameEnd;
        m_mouseEvents = 0;

        m_hasNewStats = true;
    }
}