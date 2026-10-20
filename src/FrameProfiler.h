#pragma once

// Header-only frame profiler for AgarClient (C++17/20).
//
// Sequential frame stages (mark these in main.cpp in execution order):
//   beginFrame();
//   inputManager.poll(...);       mark(Input);
//   network.update();             mark(Network);
//   updateGame(...);               mark(Update);
//   gameRenderer.draw(...);
//   gameHud.draw(...);             mark(Draw);
//   window.swap();                 mark(Swap);
//   frameLimiter.endFrame();       mark(Wait);
//   endFrame();                    // optional but recommended: records FrameTotal
//
// Nested measurements (do NOT use mark() for these, since mark() moves the
// sequential stage boundary). Use a scoped timer or record() instead:
//   {
//       FrameProfiler::ScopedTimer timer(profiler, FrameProfiler::Chat);
//       chatHUD.draw(...);
//   }
//   // or: profiler.record(FrameProfiler::Chat, elapsedMilliseconds);
//
// Suggested nested stages: GameRender, HUD, Chat. These are separate views of
// work already included in Draw, so NEVER add their times to Draw.
//
// Stats are calculated from the most recent up-to-1024 samples. stat() sorts a
// fixed-size local array and does not allocate heap memory. Call it infrequently
// (e.g. once per second), not every frame. This class is intended for use from
// the render/main thread; it is not internally synchronized.

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <string_view>

class FrameProfiler
{
public:
    // Keep the original six stage names first, in their original order, so
    // existing FrameProfiler::Input/Network/... references remain compatible.
    enum Stage : std::size_t
    {
        Input,
        Network,
        Update,
        Draw,
        Swap,
        Wait,

        // Independent/nested measurements.
        GameRender,
        HUD,
        Chat,
        ChatPrepare,
        ChatVisibility,
        ChatRender,
        ChatVerifiedMarks,
        Leaderboard,
        PlayerHUD,
        TextEnd,

        // Automatically recorded by endFrame().
        FrameTotal,

        // CPU-side timing of the chat text renderer.
        ChatTextBuild,
        ChatTextFlush,

        // Внутренние этапы GameRenderer.
        GameRenderSplitSimulate,
        GameRenderPrepare,
        GameRenderSort,
        GameRenderSkinLookup,
        GameRenderShaderSetup,
        GameRenderInstanced,
        GameRenderEntities,
        GameRenderSplitPreview,
        GameRenderTextFlush,
        GameRenderEntityOutlines,
        GameRenderSkinDraw,
        GameRenderTextBuild,
        Count
    };

    struct Stat
    {
        double p50 = 0.0;
        double p95 = 0.0;
        double p99 = 0.0;
        double average = 0.0;
        double max = 0.0;
        std::size_t samples = 0; // Number of samples in this rolling window.
    };

    class ScopedTimer
    {
    public:
        ScopedTimer(FrameProfiler& profiler, Stage stage) noexcept
            : m_profiler(profiler),
              m_stage(stage),
              m_start(Clock::now())
        {
        }

        ~ScopedTimer() noexcept
        {
            const auto elapsed = Clock::now() - m_start;
            m_profiler.record(
                m_stage,
                std::chrono::duration<double, std::milli>(elapsed).count());
        }

        ScopedTimer(const ScopedTimer&) = delete;
        ScopedTimer& operator=(const ScopedTimer&) = delete;
        ScopedTimer(ScopedTimer&&) = delete;
        ScopedTimer& operator=(ScopedTimer&&) = delete;

    private:
        using Clock = std::chrono::steady_clock;

        FrameProfiler& m_profiler;
        Stage m_stage;
        Clock::time_point m_start;
    };

    void beginFrame() noexcept
    {
        const auto now = Clock::now();
        m_frameStart = now;
        m_lastMark = now;
        m_frameStarted = true;
    }

    // Records the time since beginFrame() or the previous mark(). Use only for
    // the sequential top-level stages in the main loop.
    void mark(Stage stage) noexcept
    {
        const auto now = Clock::now();
        record(stage,
            std::chrono::duration<double, std::milli>(now - m_lastMark).count());
        m_lastMark = now;
    }

    // Records an independent/nested duration in milliseconds. Does not affect
    // the sequential mark() clock.
    void record(Stage stage, double milliseconds) noexcept
    {
        const auto index = static_cast<std::size_t>(stage);
        if (index >= Count || !std::isfinite(milliseconds) || milliseconds < 0.0)
            return;

        m_history[index][m_sampleCount[index] % kWindow] =
            static_cast<float>(milliseconds);
        ++m_sampleCount[index];
    }

    // Records total wall-clock time from beginFrame() to this call. Call after
    // mark(Wait), if the frame limiter's waiting time should be included.
    void endFrame() noexcept
    {
        if (!m_frameStarted)
            return;

        record(FrameTotal,
            std::chrono::duration<double, std::milli>(
                Clock::now() - m_frameStart).count());
        m_frameStarted = false;
    }

    // Rolling-window percentiles. Recommended once per second, not per frame.
    [[nodiscard]] Stat stat(Stage stage) const noexcept
    {
        const auto stageIndex = static_cast<std::size_t>(stage);
        if (stageIndex >= Count)
            return {};

        const std::size_t total = m_sampleCount[stageIndex];
        const std::size_t n = (total < kWindow) ? total : kWindow;
        if (n == 0)
            return {};

        std::array<float, kWindow> values{};
        const std::size_t start = (total >= kWindow) ? (total % kWindow) : 0;
        double sum = 0.0;

        for (std::size_t i = 0; i < n; ++i)
        {
            const float value = m_history[stageIndex][(start + i) % kWindow];
            values[i] = value;
            sum += value;
        }

        std::sort(values.begin(), values.begin() + n);

        const auto percentileIndex = [n](double percentile) noexcept
        {
            // Nearest-rank percentile: ceil(p * n) - 1, clamped to [0, n-1].
            const auto rank = static_cast<std::size_t>(std::ceil(percentile * n));
            return (rank == 0) ? std::size_t{0}
                               : std::min(n - 1, rank - 1);
        };

        Stat result;
        result.samples = n;
        result.p50 = values[percentileIndex(0.50)];
        result.p95 = values[percentileIndex(0.95)];
        result.p99 = values[percentileIndex(0.99)];
        result.max = values[n - 1];
        result.average = sum / static_cast<double>(n);
        return result;
    }

    [[nodiscard]] static constexpr std::string_view stageName(Stage stage) noexcept
    {
        switch (stage)
        {
        case Input:      return "Input";
        case Network:    return "Network";
        case Update:     return "Update";
        case Draw:       return "Draw";
        case Swap:       return "Swap";
        case Wait:       return "Wait";
        case GameRender: return "GameRender";
        case HUD:        return "HUD";
        case Chat:       return "Chat";
        case ChatPrepare:       return "ChatPrepare";
        case ChatVisibility:    return "ChatVisibility";
        case ChatRender:        return "ChatRender";
        case ChatVerifiedMarks: return "ChatVerifiedMarks";
        case FrameTotal: return "FrameTotal";
        case Leaderboard: return "Leaderboard";
        case PlayerHUD:   return "PlayerHUD";
        case TextEnd:     return "TextEnd";
        case ChatTextBuild: return "ChatTextBuild";
        case ChatTextFlush: return "ChatTextFlush";
        case GameRenderSplitSimulate: return "GR_SplitSim";
        case GameRenderPrepare:       return "GR_Prepare";
        case GameRenderSort:          return "GR_Sort";
        case GameRenderSkinLookup:    return "GR_SkinLookup";
        case GameRenderShaderSetup:   return "GR_ShaderSetup";
        case GameRenderInstanced:     return "GR_Instanced";
        case GameRenderEntities:      return "GR_Entities";
        case GameRenderSplitPreview:  return "GR_SplitPreview";
        case GameRenderTextFlush:     return "GR_TextFlush";
        case GameRenderEntityOutlines: return "GR_EntityOutlines";
        case GameRenderSkinDraw:       return "GR_SkinDraw";
        case GameRenderTextBuild:      return "GR_TextBuild";
        case Count:      break;
        }
        return "Unknown";
    }

    static constexpr std::size_t windowSize() noexcept { return kWindow; }

private:
    using Clock = std::chrono::steady_clock;
    static constexpr std::size_t kWindow = 1024;

    std::array<std::array<float, kWindow>, Count> m_history{};
    std::array<std::size_t, Count> m_sampleCount{};
    Clock::time_point m_frameStart = Clock::now();
    Clock::time_point m_lastMark = m_frameStart;
    bool m_frameStarted = false;
};
