#include "FrameLimiter.h"

#include <algorithm>
#include <thread>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#endif
#include <winsock2.h>
#include <windows.h>
#include <timeapi.h>
#include <avrt.h>

#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "avrt.lib")

#ifndef CREATE_WAITABLE_TIMER_HIGH_RESOLUTION
#define CREATE_WAITABLE_TIMER_HIGH_RESOLUTION 0x00000002
#endif
#ifndef PROCESS_POWER_THROTTLING_IGNORE_TIMER_RESOLUTION
#define PROCESS_POWER_THROTTLING_IGNORE_TIMER_RESOLUTION 0x4
#endif
#endif

#if defined(_M_X64) || defined(__x86_64__) || defined(_M_IX86) || defined(__i386__)
#include <immintrin.h>
#define FL_CPU_RELAX() _mm_pause()
#else
#define FL_CPU_RELAX() std::this_thread::yield()
#endif

namespace
{
    // Границы адаптивного запаса: за сколько до цели мы просыпаемся
    // из таймера и дожидаемся остатка busy-wait'ом.
    constexpr std::chrono::nanoseconds kMinMargin{ 200'000 };   // 0.2 мс
    constexpr std::chrono::nanoseconds kMaxMargin{ 3'000'000 }; // 3 мс
    constexpr std::chrono::nanoseconds kSlack{ 150'000 };       // 0.15 мс сверху
}

FrameLimiter::FrameLimiter()
{
    setTargetFps(m_fps);

#ifdef _WIN32
    // 1 мс для всего процесса: помогает SDL_Delay, sleep'ам сетевого потока и т.д.
    timeBeginPeriod(1);
    m_periodSet = true;

    m_timer = CreateWaitableTimerExW(
        nullptr, nullptr,
        CREATE_WAITABLE_TIMER_HIGH_RESOLUTION,
        TIMER_ALL_ACCESS);

    // Windows 11 игнорирует timeBeginPeriod для свёрнутых/перекрытых окон,
    // а также может троттлить процесс. Просим этого не делать.
    PROCESS_POWER_THROTTLING_STATE pt{};
    pt.Version = PROCESS_POWER_THROTTLING_CURRENT_VERSION;
    pt.ControlMask = PROCESS_POWER_THROTTLING_EXECUTION_SPEED
        | PROCESS_POWER_THROTTLING_IGNORE_TIMER_RESOLUTION;
    pt.StateMask = 0; // 0 = «троттлинг выключен»
    SetProcessInformation(GetCurrentProcess(), ProcessPowerThrottling, &pt, sizeof(pt));

    // MMCSS: планировщик стабильнее выдаёт кванты потоку игры.
    DWORD taskIndex = 0;
    m_mmcss = AvSetMmThreadCharacteristicsW(L"Games", &taskIndex);
#endif
}

FrameLimiter::~FrameLimiter()
{
#ifdef _WIN32
    if (m_mmcss)     AvRevertMmThreadCharacteristics(m_mmcss);
    if (m_timer)     CloseHandle(m_timer);
    if (m_periodSet) timeEndPeriod(1);
#endif
}

void FrameLimiter::setTargetFps(int fps)
{
    m_fps = fps;
    if (fps > 0)
        m_frame = Ns(1'000'000'000LL / fps);
    m_started = false;
}

void FrameLimiter::sleepUntil(Clock::time_point t)
{
    // Просыпаемся заранее на m_margin, остаток добираем spin'ом.
    const auto coarse = t - m_margin;
    const auto remaining = coarse - Clock::now();

    if (remaining > Ns::zero())
    {
#ifdef _WIN32
        if (m_timer)
        {
            LARGE_INTEGER due{};
            // Отрицательное значение = относительное время, единицы по 100 нс.
            due.QuadPart = -(std::chrono::duration_cast<Ns>(remaining).count() / 100);
            if (SetWaitableTimer(m_timer, &due, 0, nullptr, nullptr, FALSE))
                WaitForSingleObject(m_timer, INFINITE);
        }
        else if (remaining > std::chrono::milliseconds(2))
        {
            std::this_thread::sleep_for(remaining - std::chrono::milliseconds(1));
        }
#else
        std::this_thread::sleep_for(remaining);
#endif

        // Адаптивный запас: насколько таймер проспал сверх запрошенного?
        // Быстро растём при промахе, медленно убываем в спокойные периоды.
        const auto overshoot = std::chrono::duration_cast<Ns>(Clock::now() - coarse);
        const Ns needed = overshoot + kSlack;

        if (needed > m_margin)
            m_margin = std::min<Ns>(needed, kMaxMargin);
        else
            m_margin -= (m_margin - std::max<Ns>(needed, kMinMargin)) / 32;
    }

    while (Clock::now() < t)
        FL_CPU_RELAX();
}

void FrameLimiter::endFrame()
{
    m_lastWaitMs = 0.0;

    if (m_fps <= 0)
    {
        m_started = false;
        return;
    }

    const auto begin = Clock::now();
    if (!m_started)
    {
        m_started = true;
        m_next = begin;
    }

    m_next += m_frame;

    // Опоздали: без «догоняющих» кадров подряд, просто начинаем сетку заново.
    if (begin >= m_next)
    {
        m_next = begin;
        return;
    }

    sleepUntil(m_next);

    m_lastWaitMs = std::chrono::duration<double, std::milli>(Clock::now() - begin).count();
}