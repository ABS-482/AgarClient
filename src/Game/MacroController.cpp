#include "MacroController.h"

#include <cmath>
#include <numbers>
#include <iostream>
#include <iomanip>

MacroController::MacroController(
    NetworkClient& network
)
    : network(network),
    workerThread(&MacroController::workerLoop, this)
{
}

MacroController::~MacroController()
{
    {
        std::lock_guard<std::mutex> lock(mutex);

        stopRequested = true;
        active = false;
    }

    condition.notify_one();

    if (workerThread.joinable())
    {
        workerThread.join();
    }
}

bool MacroController::captureSnapshot(
    const WorldSnapshot& blobs,
    const std::vector<uint32_t>& ownedIds,
    const InputState& input,
    const Camera& camera,
    float screenW,
    float screenH,
    AimSnapshot& outSnapshot
)
{
    if (ownedIds.empty())
        return false;

    float sumX = 0.0f;
    float sumY = 0.0f;
    int count = 0;

    for (uint32_t id : ownedIds)
    {
        auto it = blobs->find(id);

        if (it == blobs->end())
            continue;

        sumX += it->second.targetX;
        sumY += it->second.targetY;

        ++count;
    }

    if (count == 0)
        return false;

    outSnapshot.centerX =
        sumX / static_cast<float>(count);

    outSnapshot.centerY =
        sumY / static_cast<float>(count);

    camera.screenToWorld(
        input.mouseX,
        input.mouseY,
        outSnapshot.mouseX,
        outSnapshot.mouseY
    );

    outSnapshot.valid = true;

    return true;
}

void MacroController::requestManualEject(
    const InputState& input,
    const Camera& camera,
    float screenW,
    float screenH
)
{
    AimSnapshot snapshot;

    camera.screenToWorld(
        input.mouseX,
        input.mouseY,
        snapshot.mouseX,
        snapshot.mouseY
    );

    bool macroCurrentlyActive = false;

    {
        std::lock_guard<std::mutex> lock(mutex);

        macroCurrentlyActive = active;

        if (macroCurrentlyActive)
        {
            manualEjectSnapshot = snapshot;
            manualEjectRequested = true;
        }
    }

    if (macroCurrentlyActive)
    {
        condition.notify_one();
    }
    else
    {
        network.sendAimPosition(
            snapshot.mouseX,
            snapshot.mouseY
        );

        network.requestEjectMass();
    }
}

void MacroController::requestManualSplit(
    const InputState& input,
    const Camera& camera,
    float screenW,
    float screenH
)
{
    AimSnapshot snapshot;

    camera.screenToWorld(
        input.mouseX,
        input.mouseY,
        snapshot.mouseX,
        snapshot.mouseY
    );

    bool macroCurrentlyActive;

    {
        std::lock_guard<std::mutex> lock(mutex);

        macroCurrentlyActive = active;

        if (macroCurrentlyActive)
        {
            manualSplitSnapshot = snapshot;
            manualSplitRequested.store(
                true,
                std::memory_order_release
            );
        }
    }

    if (macroCurrentlyActive)
    {
        condition.notify_one();
    }
    else
    {
        network.sendAimPosition(
            snapshot.mouseX,
            snapshot.mouseY
        );

        network.requestSplit();
    }
}

void MacroController::buildPoints(
    const AimSnapshot& snapshot,
    float outPoints[3][2]
) const
{
    constexpr double pi =
        std::numbers::pi;

    constexpr float R =
        3500.0f;

    float dx =
        snapshot.mouseX -
        snapshot.centerX;

    float dy =
        snapshot.mouseY -
        snapshot.centerY;

    float distance =
        std::sqrt(
            dx * dx +
            dy * dy
        );

    outPoints[0][0] =
        snapshot.mouseX;

    outPoints[0][1] =
        snapshot.mouseY;

    if (distance < 0.001f)
    {
        dx = 1.0f;
        dy = 0.0f;
        distance = 0.0f;
    }
    else
    {
        dx /= distance;
        dy /= distance;
    }

    const double mouseAngle =
        std::atan2(
            static_cast<double>(dy),
            static_cast<double>(dx)
        );

    const float clampedDistance =
        std::min(
            distance,
            2.0f * R
        );

    const double halfAngle =
        (clampedDistance < 0.001f)
        ? pi / 2.0
        : std::acos(
            static_cast<double>(
                clampedDistance
                ) /
            (2.0 * static_cast<double>(R))
        );

    const double oppositeAngle =
        mouseAngle + pi;

    const double angle1 =
        oppositeAngle + halfAngle;

    const double angle2 =
        oppositeAngle - halfAngle;

    outPoints[1][0] =
        snapshot.centerX +
        R *
        static_cast<float>(
            std::cos(angle1)
            );

    outPoints[1][1] =
        snapshot.centerY +
        R *
        static_cast<float>(
            std::sin(angle1)
            );

    outPoints[2][0] =
        snapshot.centerX +
        R *
        static_cast<float>(
            std::cos(angle2)
            );

    outPoints[2][1] =
        snapshot.centerY +
        R *
        static_cast<float>(
            std::sin(angle2)
            );
}

void MacroController::setQHeld(bool held)
{
    qHeld.store(
        held,
        std::memory_order_release
    );
}

void MacroController::update(
    const InputState& input,
    const WorldSnapshot& blobs,
    const std::vector<uint32_t>& ownedIds,
    const Camera& camera,
    float screenW,
    float screenH
)
{
    AimSnapshot snapshot;

    const bool valid =
        input.action(GameAction::MacroE).held &&
        captureSnapshot(
            blobs,
            ownedIds,
            input,
            camera,
            screenW,
            screenH,
            snapshot
        );

    {
        std::lock_guard<std::mutex> lock(mutex);

        if (!valid)
        {
            active = false;
            latestSnapshot.valid = false;
        }
        else
        {
            latestSnapshot = snapshot;

            if (!active)
            {
                active = true;
                currentPoint = 0;
            }
        }
    }

    condition.notify_one();
}

void MacroController::workerLoop()
{
    std::unique_lock<std::mutex> lock(mutex);

    while (!stopRequested)
    {
        condition.wait(
            lock,
            [this]
            {
                return
                    stopRequested ||
                    active;
            }
        );

        if (stopRequested)
            break;

        if (!active || !latestSnapshot.valid)
            continue;

        AimSnapshot snapshot =
            latestSnapshot;

        float localPoints[3][2]{};

        buildPoints(
            snapshot,
            localPoints
        );

        currentPoint = 0;

        while (
            active &&
            !stopRequested
            )
        {

            bool useManualEjectTarget = false;
            bool useManualSplitTarget = false;

            AimSnapshot manualEjectSnap;
            AimSnapshot manualSplitSnap;

            if (manualEjectRequested.load(
                std::memory_order_acquire))
            {
                manualEjectSnap =
                    manualEjectSnapshot;

                manualEjectRequested.store(
                    false,
                    std::memory_order_release
                );

                useManualEjectTarget = true;
            }

            if (manualSplitRequested.load(
                std::memory_order_acquire))
            {
                manualSplitSnap =
                    manualSplitSnapshot;

                manualSplitRequested.store(
                    false,
                    std::memory_order_release
                );

                useManualSplitTarget = true;
            }

            AimSnapshot manualSnapshot;

            if (useManualEjectTarget)
            {
                manualSnapshot =
                    manualEjectSnap;
            }
            else if (useManualSplitTarget)
            {
                manualSnapshot =
                    manualSplitSnap;
            }

            const bool useManualTarget =
                useManualEjectTarget ||
                useManualSplitTarget;

            const float sendX =
                useManualTarget
                ? manualSnapshot.mouseX
                : localPoints[currentPoint][0];

            const float sendY =
                useManualTarget
                ? manualSnapshot.mouseY
                : localPoints[currentPoint][1];

            lock.unlock();

            network.sendAimPosition(
                sendX,
                sendY
            );

            if (useManualEjectTarget)
            {
                network.requestEjectMass();
            }

            if (useManualSplitTarget)
            {
                network.requestSplit();
            }

            if (
                !useManualTarget &&
                currentPoint == 0 &&
                qHeld.load(
                    std::memory_order_acquire
                )
                )
            {
                network.requestEjectMass();
            }

            lock.lock();

            if (!active || stopRequested)
                break;

            condition.wait_for(
                lock,
                pointDelay,
                [this]
                {
                    return
                        stopRequested ||
                        !active ||
                        manualEjectRequested.load(
                            std::memory_order_acquire
                        ) ||
                        manualSplitRequested.load(
                            std::memory_order_acquire
                        );
                }
            );

            if (!active || stopRequested)
                break;

            if (
                manualEjectRequested.load(
                    std::memory_order_acquire
                ) ||
                manualSplitRequested.load(
                    std::memory_order_acquire
                )
                )
            {
                continue;
            }

            ++currentPoint;
            if (currentPoint >= 3)
            {
                currentPoint = 0;

                if (latestSnapshot.valid)
                {
                    snapshot =
                        latestSnapshot;

                    buildPoints(
                        snapshot,
                        localPoints
                    );
                }
            }
        }
    }
}