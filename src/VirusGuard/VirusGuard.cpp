#include "VirusGuard.h"

#include "../Game/World.h"

#include <numbers>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>

VirusGuard::VirusGuard(World& world)
    : m_world(world)
{
}

void VirusGuard::update(
    float cursorX,
    float cursorY
)
{
    m_cursorX = cursorX;
    m_cursorY = cursorY;

    const auto now = std::chrono::steady_clock::now();

    // Аналог delta из оригинального VirusGuard.
    float deltaTime = 1.0f / 60.0f;

    if (m_hasUpdateTime)
    {
        const std::chrono::duration<float> elapsed =
            now - m_lastUpdateTime;

        const float measuredDelta = elapsed.count();

        if (measuredDelta > 0.0f && measuredDelta < 0.25f)
            deltaTime = measuredDelta;
    }

    m_delta = deltaTime;

    m_massSpent -=
        budgetPerSec() * deltaTime;

    if (m_massSpent < 0.0f)
        m_massSpent = 0.0f;

    m_lastUpdateTime = now;
    m_hasUpdateTime = true;

    const WorldSnapshot snapshot = m_world.snapshot();

    const std::vector<uint32_t> ownedIds =
        m_world.getOwnedIds();

    m_ownCells.clear();

    m_maxOwnRadius = 0.0f;
    m_maxOwnMass = 0.0f;
    m_totalOwnMass = 0.0f;
    m_enemyCells.clear();

    for (uint32_t ownedId : ownedIds)
    {
        auto it = snapshot->find(ownedId);

        if (it == snapshot->end())
            continue;

        const Blob& blob = it->second;

        OwnCell cell;

        cell.id = blob.id;
        cell.x = blob.targetX;
        cell.y = blob.targetY;
        cell.radius = blob.targetSize;

        m_ownCells.emplace(cell.id, cell);

        if (cell.radius > m_maxOwnRadius)
            m_maxOwnRadius = cell.radius;

        const float mass =
            cell.radius * cell.radius / 100.0f;

        m_totalOwnMass += mass;

        if (mass > m_maxOwnMass)
            m_maxOwnMass = mass;
    }

    for (const auto& [id, blob] : *snapshot)
    {
        if (blob.cellType != CellType::Player)
            continue;

        bool isOwned = false;

        for (uint32_t ownedId : ownedIds)
        {
            if (ownedId == id)
            {
                isOwned = true;
                break;
            }
        }

        if (isOwned)
            continue;

        EnemyCell cell;

        cell.id = id;
        cell.x = blob.targetX;
        cell.y = blob.targetY;
        cell.radius = blob.targetSize;

        m_enemyCells.emplace(id, cell);
    }

    // В начале update() все старые треки считаются ненайденными.
    for (auto& [id, virus] : m_viruses)
    {
        virus.seenThisUpdate = false;

        if (virus.selfAge > 0.0f)
        {
            virus.selfAge -= deltaTime;

            if (virus.selfAge < 0.0f)
                virus.selfAge = 0.0f;
        }

        if (virus.ownFedAge > 0.0f)
        {
            virus.ownFedAge -= deltaTime;

            if (virus.ownFedAge < 0.0f)
                virus.ownFedAge = 0.0f;
        }
    }

    if (m_ownEjectAge > 0.0f)
    {
        m_ownEjectAge -= deltaTime;

        if (m_ownEjectAge < 0.0f)
            m_ownEjectAge = 0.0f;
    }

    if (m_guardFeedAge > 0.0f)
    {
        m_guardFeedAge -= deltaTime;

        if (m_guardFeedAge < 0.0f)
            m_guardFeedAge = 0.0f;
    }

    for (const auto& [id, blob] : *snapshot)
    {
        if (!blob.isVirus)
            continue;

        auto it = m_viruses.find(blob.id);

        const float x = blob.targetX;
        const float y = blob.targetY;
        const float radius = blob.targetSize;

        if (it == m_viruses.end())
        {
            VirusTrack virus;

            virus.id = blob.id;

            virus.x = x;
            virus.y = y;
            virus.radius = radius;

            virus.mass = radius * radius / 100.0f;
            virus.feeds = 0.0f;
            virus.feedAge = 0.0f;

            virus.refX = x;
            virus.refY = y;
            virus.refAge = 0.0f;

            virus.step = 0.0f;
            virus.inbound = std::numeric_limits<float>::max();
            virus.selfAge = 0.0f;
            virus.ownFedAge = 0.0f;

            virus.shots = 0;
            virus.idleUntil = 0;

            virus.arrived = false;
            virus.shotAcc = 0.0f;

            virus.velocityX = 0.0f;
            virus.velocityY = 0.0f;
            virus.hasVelocity = false;

            virus.seenThisUpdate = true;

            virus.interceptX = x;
            virus.interceptY = y;
            virus.interceptSolved = false;

            m_viruses.emplace(blob.id, virus);
        }
        else
        {
            VirusTrack& virus = it->second;

            // ------------------------------------------------------------
            // 1. Скорость через опорную точку refX/refY
            // ------------------------------------------------------------

            const float dx = x - virus.refX;
            const float dy = y - virus.refY;

            const float distance =
                std::sqrt(dx * dx + dy * dy);

            if (distance > m_config.minStepDist)
            {
                const float sampleTime =
                    virus.refAge + deltaTime;

                if (sampleTime > 0.001f &&
                    sampleTime < 1.0f)
                {
                    const float rawVX =
                        dx / sampleTime;

                    const float rawVY =
                        dy / sampleTime;

                    const float rawSpeed =
                        std::sqrt(
                            rawVX * rawVX +
                            rawVY * rawVY
                        );

                    if (rawSpeed <= m_config.maxVirusSpeed)
                    {
                        const float currentSpeed =
                            std::sqrt(
                                virus.velocityX * virus.velocityX +
                                virus.velocityY * virus.velocityY
                            );

                        if (!virus.hasVelocity ||
                            (currentSpeed < m_config.minFlySpeed &&
                                rawSpeed >= m_config.minFlySpeed))
                        {
                            virus.velocityX = rawVX;
                            virus.velocityY = rawVY;
                            virus.hasVelocity = true;
                        }
                        else
                        {
                            virus.velocityX +=
                                (rawVX - virus.velocityX) *
                                m_config.velEma;

                            virus.velocityY +=
                                (rawVY - virus.velocityY) *
                                m_config.velEma;
                        }

                        virus.step = distance;
                    }
                }

                virus.refX = x;
                virus.refY = y;
                virus.refAge = 0.0f;
            }
            else
            {
                virus.refAge += deltaTime;

                if (virus.refAge > m_config.velHold)
                {
                    virus.velocityX *= 0.5f;
                    virus.velocityY *= 0.5f;

                    const float speedSquared =
                        virus.velocityX * virus.velocityX +
                        virus.velocityY * virus.velocityY;

                    if (speedSquared < 4.0f)
                    {
                        virus.velocityX = 0.0f;
                        virus.velocityY = 0.0f;
                        virus.step = 0.0f;
                        virus.shots = 0;
                    }
                }
            }

            // ------------------------------------------------------------
            // 2. Подкормки
            // ------------------------------------------------------------

            const float newMass =
                radius * radius / 100.0f;

            virus.feedAge += deltaTime;

            virus.feeds -=
                deltaTime /
                std::max(m_config.feedForget, 0.05f);

            if (virus.feeds < 0.0f)
                virus.feeds = 0.0f;

            if (newMass > virus.mass + 1.0f)
            {
                if (virus.selfAge <= 0.0f &&
                    m_ownEjectAge <= 0.0f &&
                    m_guardFeedAge <= 0.0f)
                {
                    virus.feeds += 1.0f;
                }

                virus.feeds =
                    std::min(
                        virus.feeds,
                        m_config.minFeeds + 2.0f
                    );

                virus.feedAge = 0.0f;
                virus.mass = newMass;
            }
            else if (newMass < virus.mass - 1.0f)
            {
                virus.mass = newMass;
                virus.shots = 0;
            }

            if (virus.feeds >= m_config.minFeeds &&
                virus.feedAge < 0.3f &&
                virus.shots >= m_config.armedShots)
            {
                virus.shots = 0;
            }

            // ------------------------------------------------------------
            // 3. Текущая позиция
            // ------------------------------------------------------------

            virus.x = x;
            virus.y = y;
            virus.radius = radius;

            if (virus.inbound <
                1000000.0f)
            {
                virus.inbound += deltaTime;
            }

            virus.seenThisUpdate = true;
        }
    }

    // Удаляем вирусы, исчезнувшие из snapshot.
    for (auto it = m_viruses.begin();
        it != m_viruses.end();)
    {
        if (!it->second.seenThisUpdate)
            it = m_viruses.erase(it);
        else
            ++it;
    }
}

float VirusGuard::distanceToNearestCell(float x, float y) const
{
    float nearestDistance =
        std::numeric_limits<float>::max();

    for (const auto& [id, cell] : m_ownCells)
    {
        const float dx = cell.x - x;
        const float dy = cell.y - y;

        const float distance =
            std::sqrt(dx * dx + dy * dy) -
            cell.radius;

        if (distance < nearestDistance)
            nearestDistance = distance;
    }

    return nearestDistance;
}

const VirusGuard::OwnCell* VirusGuard::nearestOwnCell(
    float x,
    float y
) const
{
    const OwnCell* nearest = nullptr;

    float nearestDistanceSquared =
        std::numeric_limits<float>::max();

    for (const auto& [id, cell] : m_ownCells)
    {
        const float dx = cell.x - x;
        const float dy = cell.y - y;

        const float distanceSquared =
            dx * dx + dy * dy;

        if (distanceSquared < nearestDistanceSquared)
        {
            nearestDistanceSquared = distanceSquared;
            nearest = &cell;
        }
    }

    return nearest;
}

float VirusGuard::virusTravel(
    const VirusTrack& virus,
    float speed,
    float time
) const
{
    (void)virus;

    const float tau = m_config.virusDecelTau;

    if (tau <= 0.02f)
        return speed * time;

    return speed * tau *
        (1.0f - std::exp(-time / tau));
}

float VirusGuard::dangerTime(
    const VirusTrack& virus
) const
{
    float nearestTime =
        std::numeric_limits<float>::max();

    const float virusSpeed =
        std::sqrt(
            virus.velocityX * virus.velocityX +
            virus.velocityY * virus.velocityY
        );

    for (const auto& [id, ownCell] : m_ownCells)
    {
        if (ownCell.radius <= virus.radius + 4.0f)
            continue;

        const float dangerRadius =
            ownCell.radius +
            virus.radius +
            m_config.dangerMargin;

        const float dx =
            virus.x - ownCell.x;

        const float dy =
            virus.y - ownCell.y;

        const float currentDistance =
            std::sqrt(dx * dx + dy * dy);

        // Уже внутри опасной зоны.
        if (currentDistance <= dangerRadius)
            return 0.0f;

        // Практически не движется — прогнозировать нечего.
        if (virusSpeed < 1.0f)
            continue;

        const float dirX =
            virus.velocityX / virusSpeed;

        const float dirY =
            virus.velocityY / virusSpeed;

        float previousDistance = currentDistance;

        for (int step = 1; step <= 24; ++step)
        {
            const float time =
                m_config.lookahead *
                static_cast<float>(step) /
                24.0f;

            const float travel =
                virusTravel(
                    virus,
                    virusSpeed,
                    time
                );

            const float predictedX =
                virus.x + dirX * travel;

            const float predictedY =
                virus.y + dirY * travel;

            const float predictedDX =
                predictedX - ownCell.x;

            const float predictedDY =
                predictedY - ownCell.y;

            const float predictedDistance =
                std::sqrt(
                    predictedDX * predictedDX +
                    predictedDY * predictedDY
                );

            if (predictedDistance <= dangerRadius)
            {
                const float stepTime =
                    m_config.lookahead / 24.0f;

                const float denominator =
                    std::max(
                        previousDistance - predictedDistance,
                        0.001f
                    );

                float fraction =
                    (previousDistance - dangerRadius) /
                    denominator;

                fraction =
                    std::clamp(
                        fraction,
                        0.0f,
                        1.0f
                    );

                const float hitTime =
                    time -
                    stepTime +
                    stepTime * fraction;

                if (hitTime < nearestTime)
                    nearestTime = hitTime;

                break;
            }

            // Вирус начал удаляться; дальше этот проход
            // уже не интересен для данной клетки.
            if (step > 3 &&
                predictedDistance > previousDistance + 2.0f)
            {
                break;
            }

            previousDistance = predictedDistance;
        }
    }

    return nearestTime;
}

float VirusGuard::pelletReach() const
{
    float reach = m_config.pelletMax;

    if (reach < m_config.minReach)
        reach = m_config.minReach;

    if (reach > m_config.maxReach)
        reach = m_config.maxReach;

    return reach;
}

float VirusGuard::ejectTravel(float time) const
{
    const float reach = pelletReach();
    const float tau =
        std::max(m_config.pelletTau, 0.05f);

    return reach *
        (1.0f - std::exp(-time / tau));
}

bool VirusGuard::solve()
{
    const auto now =
        std::chrono::steady_clock::now();

    const long long nowMs =
        std::chrono::duration_cast<
        std::chrono::milliseconds
        >(
            now.time_since_epoch()
        ).count();

    // Каждый новый solve сначала не имеет выбранной цели.
    m_hasTarget = false;
    m_targetId = 0;

    m_targetX = 0.0f;
    m_targetY = 0.0f;

    m_targetInterceptX = 0.0f;
    m_targetInterceptY = 0.0f;

    m_targetDangerTime =
        std::numeric_limits<float>::max();

    updateCenter();
    updateVirusNearby();
    markPlayerFed();

    if (m_ownCells.empty())
        return false;

    if (m_viruses.empty())
        return false;

    if (m_maxOwnMass < m_config.minOwnMass)
        return false;

    const float maxTargetRange =
        feedRange();

    // ------------------------------------------------------------
    // Лучший обычный кандидат.
    // ------------------------------------------------------------

    uint32_t bestId = 0;

    float bestDangerTime =
        std::numeric_limits<float>::max();

    VirusTrack* bestVirus = nullptr;

    // ------------------------------------------------------------
    // Лучший sticky-кандидат.
    // ------------------------------------------------------------

    uint32_t stickyCandidateId = 0;

    float stickyDangerTime =
        std::numeric_limits<float>::max();

    VirusTrack* stickyVirus = nullptr;

    // ------------------------------------------------------------
    // Поиск целей.
    // ------------------------------------------------------------

    for (auto& [id, virus] : m_viruses)
    {
        // Вирус недавно получил массу от нас —
        // временно не рассматриваем его.
        if (virus.ownFedAge > 0.0f)
            continue;

        // --------------------------------------------------------
        // Проверяем arrived.
        // --------------------------------------------------------

        bool touching = false;
        bool farAway = true;

        const float feedRangeValue =
            feedRange();

        const float resetRangeSquared =
            feedRangeValue *
            feedRangeValue *
            2.25f;

        for (const auto& [cellId, ownCell] : m_ownCells)
        {
            const float dx =
                ownCell.x - virus.x;

            const float dy =
                ownCell.y - virus.y;

            const float distanceSquared =
                dx * dx +
                dy * dy;

            const float combinedRadius =
                ownCell.radius +
                virus.radius;

            if (
                distanceSquared <=
                combinedRadius * combinedRadius
                )
            {
                touching = true;
            }

            if (
                distanceSquared <=
                resetRangeSquared
                )
            {
                farAway = false;
            }
        }

        if (touching)
        {
            virus.arrived = true;
        }
        else if (farAway)
        {
            virus.arrived = false;
        }

        if (virus.arrived)
            continue;

        // --------------------------------------------------------
        // Скорость.
        // --------------------------------------------------------

        const float speed =
            std::sqrt(
                virus.velocityX *
                virus.velocityX +
                virus.velocityY *
                virus.velocityY
            );

        const bool fastEnough =
            speed >= m_config.minFlySpeed;

        // --------------------------------------------------------
        // Опасность.
        // --------------------------------------------------------

        const float danger =
            dangerTime(virus);

        // --------------------------------------------------------
        // Armed threat.
        // --------------------------------------------------------

        const bool armed =
            m_config.armedMode &&
            armedThreat(virus) <
            std::numeric_limits<float>::max();

        // Для обычной цели нужен и threat,
        // и достаточная скорость.
        if (
            !armed &&
            (
                danger ==
                std::numeric_limits<float>::max() ||
                !fastEnough
                )
            )
        {
            continue;
        }

        // --------------------------------------------------------
        // Intercept.
        // --------------------------------------------------------

        intercept(
            virus,
            speed
        );

        const float interceptDistance =
            distanceToNearestCell(
                virus.interceptX,
                virus.interceptY
            );

        if (
            interceptDistance >
            maxTargetRange
            )
        {
            continue;
        }

        // --------------------------------------------------------
        // Standing virus: пауза после серии кормлений.
        // --------------------------------------------------------

        if (
            !fastEnough &&
            virus.shots >=
            std::max(
                neededEjects(virus),
                4
            )
            )
        {
            if (
                virus.idleUntil != 0 &&
                nowMs >= virus.idleUntil
                )
            {
                // Пауза закончилась.
                virus.shots = 0;
                virus.idleUntil = 0;
            }
            else
            {
                // Пауза ещё не была создана.
                if (virus.idleUntil == 0)
                {
                    virus.idleUntil =
                        nowMs +
                        static_cast<long long>(
                            m_config.standingPauseMs
                            );
                }

                continue;
            }
        }

        // --------------------------------------------------------
        // Score.
        // --------------------------------------------------------

        const float score =
            armed
            ? std::min(
                danger ==
                std::numeric_limits<float>::max()
                ? 0.3f
                : danger,
                0.4f
            )
            : danger;

        // --------------------------------------------------------
        // Обычная лучшая цель.
        // --------------------------------------------------------

        if (score < bestDangerTime)
        {
            bestDangerTime = score;
            bestId = id;
            bestVirus = &virus;
        }

        // --------------------------------------------------------
        // Sticky-кандидат.
        // --------------------------------------------------------

        if (
            id == m_stickyId &&
            score < stickyDangerTime
            )
        {
            stickyDangerTime = score;
            stickyCandidateId = id;
            stickyVirus = &virus;
        }
    }

    // ------------------------------------------------------------
    // Sticky получает приоритет, если его score не хуже
    // обычного лучшего более чем в 1.6 раза.
    // ------------------------------------------------------------

    if (
        stickyVirus != nullptr &&
        (
            bestVirus == nullptr ||
            stickyDangerTime <=
            bestDangerTime * 1.6f
            )
        )
    {
        bestId =
            stickyCandidateId;

        bestDangerTime =
            stickyDangerTime;

        bestVirus =
            stickyVirus;
    }

    // ------------------------------------------------------------
    // Подходящей цели нет.
    // ------------------------------------------------------------

    if (bestVirus == nullptr)
    {
        m_stickyId = 0;
        m_lastShotWasFeed = false;

        return false;
    }

    // ------------------------------------------------------------
    // Финальный intercept для уже выбранной цели.
    // ------------------------------------------------------------

    const float bestSpeed =
        std::sqrt(
            bestVirus->velocityX *
            bestVirus->velocityX +
            bestVirus->velocityY *
            bestVirus->velocityY
        );

    intercept(
        *bestVirus,
        bestSpeed
    );

    // ------------------------------------------------------------
    // Строим итоговую точку aim.
    //
    // aimThrough() записывает её в m_aimX/m_aimY.
    // В Java эта же точка является targetX/targetY.
    // ------------------------------------------------------------

    aimThrough(*bestVirus);

    m_targetX =
        m_aimX;

    m_targetY =
        m_aimY;

    // ------------------------------------------------------------
    // Сохраняем выбранную цель.
    // ------------------------------------------------------------

    m_hasTarget = true;

    m_stickyId =
        bestId;

    m_targetId =
        bestId;

    m_targetInterceptX =
        bestVirus->interceptX;

    m_targetInterceptY =
        bestVirus->interceptY;

    m_targetDangerTime =
        bestDangerTime;

    return true;
}

bool VirusGuard::hasTarget() const
{
    return m_hasTarget;
}

uint32_t VirusGuard::targetId() const
{
    return m_targetId;
}

float VirusGuard::targetX() const
{
    return m_targetX;
}

float VirusGuard::targetY() const
{
    return m_targetY;
}

float VirusGuard::targetInterceptX() const
{
    return m_targetInterceptX;
}

float VirusGuard::targetInterceptY() const
{
    return m_targetInterceptY;
}

float VirusGuard::targetDangerTime() const
{
    return m_targetDangerTime;
}

float VirusGuard::aimX() const
{
    return m_aimX;
}

float VirusGuard::aimY() const
{
    return m_aimY;
}

void VirusGuard::aimThrough(
    VirusTrack& virus
)
{
    const OwnCell* ownCell =
        nearestOwnCell(virus.x, virus.y);

    if (ownCell == nullptr)
    {
        m_aimX = virus.interceptX;
        m_aimY = virus.interceptY;
        return;
    }

    const float dx =
        virus.interceptX - ownCell->x;

    const float dy =
        virus.interceptY - ownCell->y;

    const float distance =
        std::sqrt(dx * dx + dy * dy);

    if (distance < 0.0001f)
    {
        m_aimX = virus.interceptX;
        m_aimY = virus.interceptY;
        return;
    }

    const float aimDistance =
        distance + m_config.aimBeyond;

    m_aimX =
        ownCell->x +
        (dx / distance) * aimDistance;

    m_aimY =
        ownCell->y +
        (dy / distance) * aimDistance;
}

bool VirusGuard::targetPredictedPosition(
    float time,
    float& outX,
    float& outY
) const
{
    if (!m_hasTarget)
        return false;

    auto it =
        m_viruses.find(m_targetId);

    if (it == m_viruses.end())
        return false;

    const VirusTrack& virus =
        it->second;

    const float speed =
        std::sqrt(
            virus.velocityX * virus.velocityX +
            virus.velocityY * virus.velocityY
        );

    if (speed <= 0.001f)
    {
        outX = virus.x;
        outY = virus.y;
        return true;
    }

    const float dirX =
        virus.velocityX / speed;

    const float dirY =
        virus.velocityY / speed;

    const float travel =
        virusTravel(
            virus,
            speed,
            time
        );

    outX =
        virus.x +
        dirX * travel;

    outY =
        virus.y +
        dirY * travel;

    return true;
}

float VirusGuard::feedRange() const
{
    return pelletReach() + m_config.approachMargin;
}

float VirusGuard::trackRange() const
{
    return m_maxOwnRadius +
        pelletReach() +
        m_config.approachMargin;
}

float VirusGuard::budgetPerSec() const
{
    return std::max(
        m_config.massBudgetFloor,
        m_totalOwnMass * m_config.massBudgetShare
    );
}

void VirusGuard::updateVirusNearby()
{
    m_virusNearby = false;

    if (m_ownCells.empty())
        return;

    const float range =
        trackRange();

    const float rangeSquared =
        range * range;

    for (const auto& [virusId, virus] : m_viruses)
    {
        for (const auto& [cellId, cell] : m_ownCells)
        {
            const float dx =
                cell.x - virus.x;

            const float dy =
                cell.y - virus.y;

            const float distanceSquared =
                dx * dx + dy * dy;

            if (distanceSquared <= rangeSquared)
            {
                m_virusNearby = true;
                return;
            }
        }
    }
}

bool VirusGuard::virusNearby() const
{
    return m_virusNearby;
}

float VirusGuard::armedThreat(
    const VirusTrack& virus
) const
{
    constexpr float infinity =
        std::numeric_limits<float>::max();

    const float virusMass =
        virus.radius * virus.radius / 100.0f;

    // Вирус недостаточно большой.
    if (virusMass < m_config.armedMass)
        return infinity;

    // Недостаточно недавних кормлений.
    if (virus.feeds < m_config.minFeeds)
        return infinity;

    // Последнее кормление слишком старое.
    if (virus.feedAge > m_config.armedFeedFresh)
        return infinity;

    // --------------------------------------------------------
    // Ищем ближайшую вражескую клетку к вирусу.
    // --------------------------------------------------------

    const EnemyCell* nearestEnemy = nullptr;

    float nearestDistanceSquared =
        m_config.feederRange *
        m_config.feederRange;

    for (const auto& [id, enemy] : m_enemyCells)
    {
        const float dx =
            enemy.x - virus.x;

        const float dy =
            enemy.y - virus.y;

        const float distanceSquared =
            dx * dx + dy * dy;

        if (distanceSquared < nearestDistanceSquared)
        {
            nearestDistanceSquared = distanceSquared;
            nearestEnemy = &enemy;
        }
    }

    if (nearestEnemy == nullptr)
        return infinity;

    // --------------------------------------------------------
    // Направление от кормящего врага к вирусу.
    // --------------------------------------------------------

    float dx =
        virus.x - nearestEnemy->x;

    float dy =
        virus.y - nearestEnemy->y;

    const float distance =
        std::sqrt(dx * dx + dy * dy);

    if (distance < 1.0f)
        return infinity;

    const float dirX =
        dx / distance;

    const float dirY =
        dy / distance;

    // --------------------------------------------------------
    // Проверяем собственные клетки.
    // --------------------------------------------------------

    for (const auto& [id, ownCell] : m_ownCells)
    {
        // Наша клетка должна быть крупнее вируса.
        if (ownCell.radius <= virus.radius)
            continue;

        const float ownDx =
            ownCell.x - virus.x;

        const float ownDy =
            ownCell.y - virus.y;

        // Продольная координата относительно
        // направления enemy -> virus.
        const float along =
            ownDx * dirX +
            ownDy * dirY;

        if (along <= 0.0f)
            continue;

        // Клетка должна быть достаточно близко по длине.
        if (along > feedRange())
            continue;

        // Поперечное расстояние от линии enemy -> virus.
        const float perpendicularX =
            ownDx - dirX * along;

        const float perpendicularY =
            ownDy - dirY * along;

        const float perpendicularDistance =
            std::sqrt(
                perpendicularX * perpendicularX +
                perpendicularY * perpendicularY
            );

        const float allowedDistance =
            ownCell.radius +
            virus.radius * 1.5f;

        if (perpendicularDistance > allowedDistance)
            continue;

        return 0.2f;
    }

    return infinity;
}

void VirusGuard::noteOwnEject()
{
    m_ownEjectAge = m_config.selfFeedWindow;
}

void VirusGuard::noteGuardFeed()
{
    m_guardFeedAge = m_config.selfFeedWindow;

    if (!m_hasTarget)
        return;

    auto it = m_viruses.find(m_targetId);

    if (it == m_viruses.end())
        return;

    VirusTrack& virus = it->second;

    const float feedDuration =
        static_cast<float>(
            std::max(
                m_config.feedTickMs *
                std::max(m_config.stayPoints, 1),
                1
            )
            ) / 1000.0f;

    virus.selfAge =
        m_config.selfFeedWindow;

    // Накопление "выстрелов" от непрерывного кормления.
    virus.shotAcc +=
        m_delta / feedDuration;

    while (virus.shotAcc >= 1.0f)
    {
        virus.shotAcc -= 1.0f;
        virus.shots++;
    }

    // Масса расходуется пропорционально длительности кормления.
    m_massSpent +=
        m_config.ejectMass *
        static_cast<float>(
            std::max<std::size_t>(
                m_ownCells.size(),
                1
            )
            ) *
        (m_delta / feedDuration);
}

void VirusGuard::markPlayerFed()
{
    if (m_ownEjectAge <= 0.0f ||
        m_ownCells.empty())
    {
        return;
    }

    const float range =
        m_maxOwnRadius + pelletReach();

    const float immunity =
        std::max(m_config.ownFeedImmunityMs, 0) /
        1000.0f;

    for (auto& [virusId, virus] : m_viruses)
    {
        for (const auto& [cellId, cell] : m_ownCells)
        {
            const float dx = cell.x - virus.x;
            const float dy = cell.y - virus.y;

            if (dx * dx + dy * dy > range * range)
                continue;

            virus.ownFedAge = immunity;
            break;
        }
    }
}

void VirusGuard::updateCenter()
{
    if (m_ownCells.empty())
        return;

    float sumX = 0.0f;
    float sumY = 0.0f;

    for (const auto& [id, cell] : m_ownCells)
    {
        sumX += cell.x;
        sumY += cell.y;
    }

    const float count =
        static_cast<float>(m_ownCells.size());

    m_centerX = sumX / count;
    m_centerY = sumY / count;
}

bool VirusGuard::isFlying(
    const VirusTrack& virus
) const
{
    const float speedSquared =
        virus.velocityX * virus.velocityX +
        virus.velocityY * virus.velocityY;

    const float minSpeedSquared =
        m_config.minFlySpeed *
        m_config.minFlySpeed;

    return speedSquared >= minSpeedSquared;
}

bool VirusGuard::engagedAsFlying(
    const VirusTrack& virus
) const
{
    return
        isFlying(virus) ||
        virus.inbound <= m_config.recentInboundSec;
}

int VirusGuard::pelletsReaching(
    const VirusTrack& virus
) const
{
    const float reach = pelletReach();

    int count = 0;

    for (const auto& [id, ownCell] : m_ownCells)
    {
        const float dx =
            virus.interceptX - ownCell.x;

        const float dy =
            virus.interceptY - ownCell.y;

        const float distance =
            std::sqrt(dx * dx + dy * dy) -
            ownCell.radius;

        if (distance <= reach)
            ++count;
    }

    return count;
}

int VirusGuard::neededEjects(
    const VirusTrack& virus
) const
{
    if (engagedAsFlying(virus))
        return m_config.flyingBurst;

    const float mass =
        virus.radius *
        virus.radius /
        100.0f;

    const float massNeeded =
        (
            m_config.fireMass -
            mass
            ) /
        std::max(
            m_config.feedStep,
            1.0f
        );

    int needed =
        static_cast<int>(
            std::ceil(massNeeded)
            );

    if (needed < 1)
        needed = 1;

    int reaching =
        pelletsReaching(virus);

    if (reaching < 1)
        reaching = 1;

    int shots =
        static_cast<int>(
            std::ceil(
                static_cast<float>(needed) /
                static_cast<float>(reaching)
            )
            );

    if (shots < 1)
        shots = 1;

    const int maxShots =
        m_config.armedShots + 1;

    if (shots > maxShots)
        shots = maxShots;

    return shots;
}

bool VirusGuard::canEject() const
{
    if (!m_hasTarget ||
        m_maxOwnMass < m_config.minOwnMass)
    {
        return false;
    }

    auto it =
        m_viruses.find(m_targetId);

    if (it == m_viruses.end())
        return false;

    const VirusTrack& virus =
        it->second;

    // Для летающей / недавно inbound-цели
    // Java разрешает выброс независимо от massSpent.
    if (engagedAsFlying(virus))
        return true;

    // Первый выстрел всегда разрешён.
    if (virus.shots == 0)
        return true;

    return m_massSpent < budgetPerSec();
}

bool VirusGuard::planShot()
{
    m_shotEjects = false;

    if (m_ownCells.empty())
        return false;

    // Есть цель — сначала пытаемся сделать целевой выстрел.
    if (m_hasTarget && canEject())
    {
        m_shotX = m_targetX;
        m_shotY = m_targetY;

        m_lastShotWasFeed = true;
        m_shotIsFeed = true;
        m_shotEjects = true;

        return true;
    }

    // Далее — режим удержания позиции.
    if (!m_config.stayInPlace)
        return false;

    if (m_config.freezeOnlyWhileEngaging && !m_hasTarget)
        return false;

    if (!m_config.freezeOnlyWhileEngaging && !m_virusNearby)
        return false;

    if (!canEject())
        return false;

    const int points =
        std::max(m_config.stayPoints, 1);

    const float dx =
        m_aimX - m_centerX;

    const float dy =
        m_aimY - m_centerY;

    const float angle =
        dx * dx + dy * dy > 1.0e-6f
        ? std::atan2(dy, dx)
        : 0.0f;

    const float stepAngle =
        static_cast<float>(2.0 * std::numbers::pi / points);

    const float shotAngle =
        angle + stepAngle *
        static_cast<float>(m_stayStep);

    m_stayStep =
        (m_stayStep + 1) % points;

    m_shotX =
        m_centerX +
        std::cos(shotAngle) *
        m_config.stayRadius;

    m_shotY =
        m_centerY +
        std::sin(shotAngle) *
        m_config.stayRadius;

    m_lastShotWasFeed = false;
    m_shotIsFeed = false;
    m_shotEjects = false;

    return true;
}

float VirusGuard::shotX() const
{
    return m_shotX;
}

float VirusGuard::shotY() const
{
    return m_shotY;
}

bool VirusGuard::shotIsFeed() const
{
    return m_shotIsFeed;
}

bool VirusGuard::shotEjects() const
{
    return m_shotEjects;
}

void VirusGuard::noteShotFired()
{
    if (!m_hasTarget)
        return;

    auto it = m_viruses.find(m_targetId);

    if (it == m_viruses.end())
        return;

    VirusTrack& virus = it->second;

    // Каждый реальный выброс = один выстрел по вирусу.
    virus.shots++;

    // После собственного выброса вирус получает окно immunity.
    virus.selfAge = m_config.selfFeedWindow;

    // Стоимость одного выброса.
    m_massSpent +=
        static_cast<float>(std::max<std::size_t>(m_ownCells.size(), 1))
        * m_config.ejectMass;

    // Для летящей / inbound-цели Java здесь больше ничего
    // не делает и не ставит обычную reengage-паузу.
    if (engagedAsFlying(virus))
        return;

    const int requiredShots =
        std::max(
            neededEjects(virus),
            m_config.armedShots
        );

    if (virus.shots >= requiredShots)
    {
        const auto now =
            std::chrono::steady_clock::now();

        virus.idleUntil =
            std::chrono::duration_cast<
            std::chrono::milliseconds
            >(now.time_since_epoch()).count()
            + static_cast<long long>(
                m_config.reengageMs
                );

        m_stickyId = 0;
    }
}

float VirusGuard::shotCost() const
{
    return static_cast<float>(
        std::max<std::size_t>(m_ownCells.size(), 1)
        ) * m_config.ejectMass;
}

void VirusGuard::intercept(
    VirusTrack& v,
    float speed
) const
{
    v.interceptX = v.x;
    v.interceptY = v.y;
    v.interceptSolved = true;

    const OwnCell* c =
        nearestOwnCell(v.x, v.y);

    if (!c)
        return;

    const float dirX =
        speed > 1.0f
        ? v.velocityX / speed
        : 0.0f;

    const float dirY =
        speed > 1.0f
        ? v.velocityY / speed
        : 0.0f;

    for (int i = 0; i <= 20; ++i)
    {
        const float t =
            static_cast<float>(i) * 0.05f;

        const float travel =
            virusTravel(v, speed, t);

        const float px =
            v.x + dirX * travel;

        const float py =
            v.y + dirY * travel;

        const float dx =
            px - c->x;

        const float dy =
            py - c->y;

        const float distance =
            std::sqrt(dx * dx + dy * dy) -
            c->radius;

        if (distance <= ejectTravel(t))
        {
            v.interceptX = px;
            v.interceptY = py;
            return;
        }
    }

    const float travel =
        virusTravel(v, speed, 0.12f);

    v.interceptX =
        v.x + dirX * travel;

    v.interceptY =
        v.y + dirY * travel;
}