#pragma once

#include "../Input/InputState.h"
#include "../Game/World.h"
#include "../Network/NetworkClient.h"
#include "../Graphics/Camera.h"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <thread>
#include <vector>

class MacroController
{
public:
    explicit MacroController(NetworkClient& network);

    ~MacroController();

    MacroController(const MacroController&) = delete;
    MacroController& operator=(const MacroController&) = delete;

    void update(
        const InputState& input,
        const WorldSnapshot& blobs,
        const std::vector<uint32_t>& ownedIds,
        const Camera& camera,
        float screenW,
        float screenH
    );

    void requestManualEject(
        const InputState& input,
        const Camera& camera,
        float screenW,
        float screenH
    );

    void setQHeld(bool held);

    void requestManualSplit(
        const InputState& input,
        const Camera& camera,
        float screenW,
        float screenH
    );

private:
    using Clock = std::chrono::steady_clock;

    static constexpr float radius = 3500.0f;
    static constexpr auto pointDelay =
        std::chrono::duration<double, std::milli>(50.0);

    struct AimSnapshot
    {
        float centerX = 0.0f;
        float centerY = 0.0f;

        float mouseX = 0.0f;
        float mouseY = 0.0f;

        bool qHeld = false;
        bool valid = false;
    };

    std::atomic<bool> manualEjectRequested{ false };
    std::atomic<bool> qHeld{ false };

    AimSnapshot manualEjectSnapshot;

    std::atomic<bool> manualSplitRequested{ false };
    AimSnapshot manualSplitSnapshot;

    NetworkClient& network;

    std::thread workerThread;

    std::mutex mutex;
    std::condition_variable condition;

    bool stopRequested = false;
    bool active = false;

    AimSnapshot latestSnapshot;

    int currentPoint = 0;

    float points[4][2]{};

    void workerLoop();

    bool captureSnapshot(
        const WorldSnapshot& blobs,
        const std::vector<uint32_t>& ownedIds,
        const InputState& input,
        const Camera& camera,
        float screenW,
        float screenH,
        AimSnapshot& outSnapshot
    );

    bool forceP0 = false;

    void buildPoints(
        const AimSnapshot& snapshot,
        float outPoints[3][2]
    ) const;
};