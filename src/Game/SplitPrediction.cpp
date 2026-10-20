#include "SplitPrediction.h"

#include <algorithm>
#include <cmath>
#include <vector>

namespace
{
    static float distToSegment(
        float px,
        float py,
        float x1,
        float y1,
        float x2,
        float y2
    )
    {
        const float dx = x2 - x1;
        const float dy = y2 - y1;

        if (dx == 0.0f && dy == 0.0f)
        {
            return std::hypot(
                px - x1,
                py - y1
            );
        }

        float t =
            (
                (px - x1) * dx +
                (py - y1) * dy
                ) /
            (
                dx * dx +
                dy * dy
                );

        t = std::clamp(t, 0.0f, 1.0f);

        const float closestX =
            x1 + t * dx;

        const float closestY =
            y1 + t * dy;

        return std::hypot(
            px - closestX,
            py - closestY
        );
    };

    struct Sim
    {
        float x = 0.0f;
        float y = 0.0f;

        float startX = 0.0f;
        float startY = 0.0f;

        float radius = 0.0f;
        float mass = 0.0f;

        float vx = 0.0f;
        float vy = 0.0f;

        int boost = 0;
        int cooldown = 0;

        bool alive = true;
        bool predicted = false;

        int dangerLevel = 0;
        bool hitsVirus = false;
    };

    struct Target
    {
        uint32_t id = 0;

        float x = 0.0f;
        float y = 0.0f;

        float radius = 0.0f;
        float mass = 0.0f;

        bool virus = false;
        bool alive = true;
    };
}

SplitPredictionResult SplitPrediction::simulate(
    const WorldSnapshot& blobs,
    const std::vector<uint32_t>& ownedIds,
    const std::unordered_map<uint32_t, RenderState>& renderStates,
    float targetX,
    float targetY
) const
{
    SplitPredictionResult result;

    constexpr float minSplitRadius = 60.0f;

    constexpr int predictionTicks = 16;

    constexpr float impulseMultiplier = 6.0f;
    constexpr float impulseDecay = 0.85f;

    constexpr float maxMass = 32500.0f;

    constexpr std::size_t maxCells = 64;

    // ------------------------------------------------------------
    // Скорость клетки от массы.
    // ------------------------------------------------------------

    const auto speedOf =
        [](float mass)
        {
            return
                30.0f *
                std::pow(
                    mass,
                    -2.0f / 9.0f
                ) *
                (50.0f / 40.0f);
        };

    // ------------------------------------------------------------
    // Движение одной клетки на один prediction tick.
    // ------------------------------------------------------------

    const auto stepToward =
        [&](Sim& cell) -> bool
        {
            const float dx =
                targetX - cell.x;

            const float dy =
                targetY - cell.y;

            const float distance =
                std::sqrt(
                    dx * dx +
                    dy * dy
                );

            if (distance <= 0.0001f)
                return false;

            const float step =
                std::min(
                    distance,
                    speedOf(cell.mass)
                );

            cell.x +=
                dx / distance *
                step;

            cell.y +=
                dy / distance *
                step;

            return true;
        };

    // ------------------------------------------------------------
    // Собираем все исходные клетки.
    // ------------------------------------------------------------

    struct Source
    {
        const RenderState* rs = nullptr;
        float radius = 0.0f;
        float mass = 0.0f;
    };

    std::vector<Source> sources;

    sources.reserve(
        std::min(
            ownedIds.size(),
            maxCells
        )
    );

    for (uint32_t id : ownedIds)
    {
        if (sources.size() >= maxCells)
            break;

        const auto rsIt =
            renderStates.find(id);

        if (rsIt == renderStates.end())
            continue;

        const RenderState& rs =
            rsIt->second;

        const float radius =
            rs.size;

        if (radius <= 0.0f)
            continue;

        const float mass =
            std::min(
                radius * radius / 100.0f,
                maxMass
            );

        if (mass <= 0.0f)
            continue;

        sources.push_back(
            {
                &rs,
                radius,
                mass
            }
        );
    }

    const std::size_t originalCount =
        sources.size();

    // ------------------------------------------------------------
    // Собираем чужие Player-клетки и вирусы.
    //
    // Их положение берём из текущего RenderState.
    // ------------------------------------------------------------

    std::vector<Target> targets;

    targets.reserve(
        blobs->size()
    );

    for (const auto& [id, blob] : *blobs)
    {
        if (blob.cellType != CellType::Player &&
            blob.cellType != CellType::Virus)
        {
            continue;
        }

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

        const auto rsIt =
            renderStates.find(id);

        if (rsIt == renderStates.end())
            continue;

        const RenderState& rs =
            rsIt->second;

        if (rs.size <= 0.0f)
            continue;

        Target target;

        target.id =
            id;

        target.x =
            rs.x;

        target.y =
            rs.y;

        target.radius =
            rs.size;

        target.mass =
            rs.size *
            rs.size /
            100.0f;

        target.virus =
            blob.cellType == CellType::Virus;

        targets.push_back(
            target
        );
    }

    // ------------------------------------------------------------
    // Общий список симулируемых клеток.
    // ------------------------------------------------------------

    std::vector<Sim> cells;

    cells.reserve(
        std::min(
            originalCount * 2,
            maxCells
        )
    );

    std::vector<Sim> newCells;

    newCells.reserve(
        originalCount
    );

    // ------------------------------------------------------------
    // Создаём исходные клетки.
    // ------------------------------------------------------------

    for (const Source& source : sources)
    {
        const RenderState& rs =
            *source.rs;

        const float radius =
            source.radius;

        const float mass =
            source.mass;

        const bool canSplit =
            radius >= minSplitRadius &&
            originalCount +
            newCells.size() <
            maxCells;

        if (!canSplit)
        {
            Sim cell;

            cell.x =
                rs.x;

            cell.y =
                rs.y;

            cell.radius =
                radius;

            cell.mass =
                mass;

            cell.cooldown =
                0;

            cell.predicted =
                false;

            cells.push_back(
                cell
            );

            continue;
        }

        // --------------------------------------------------------
        // Направление split.
        // --------------------------------------------------------

        float dx =
            targetX - rs.x;

        float dy =
            targetY - rs.y;

        const float distance =
            std::sqrt(
                dx * dx +
                dy * dy
            );

        float dirX;
        float dirY;

        if (distance > 0.0001f)
        {
            dirX =
                dx / distance;

            dirY =
                dy / distance;
        }
        else
        {
            dirX = -1.0f;
            dirY = 0.0f;
        }

        const float halfMass =
            mass * 0.5f;

        const float halfRadius =
            std::ceil(
                std::sqrt(
                    halfMass * 100.0f
                )
            );

        // --------------------------------------------------------
        // Исходная половина.
        // --------------------------------------------------------

        Sim original;

        original.x =
            rs.x;

        original.y =
            rs.y;

        original.startX =
            rs.x;

        original.startY =
            rs.y;

        original.radius =
            halfRadius;

        original.mass =
            halfMass;

        original.cooldown =
            0;

        original.predicted =
            true;

        // --------------------------------------------------------
        // Новая половина.
        // --------------------------------------------------------

        Sim split;

        split.x =
            rs.x +
            dirX * radius;

        split.y =
            rs.y +
            dirY * radius;

        split.startX =
            split.x;

        split.startY =
            split.y;

        split.radius =
            halfRadius;

        split.mass =
            halfMass;

        split.vx =
            dirX *
            speedOf(mass) *
            impulseMultiplier;

        split.vy =
            dirY *
            speedOf(mass) *
            impulseMultiplier;

        split.boost =
            32;

        split.cooldown =
            10;

        split.predicted =
            true;

        cells.push_back(
            original
        );

        newCells.push_back(
            split
        );
    }

    // ------------------------------------------------------------
    // Новые split-клетки идут после исходных.
    // ------------------------------------------------------------

    for (const Sim& cell : newCells)
    {
        if (cells.size() >= maxCells)
            break;

        cells.push_back(
            cell
        );
    }

    // ------------------------------------------------------------
    // Проверка возможности съесть цель.
    //
    // Java:
    //
    // massA > massB * 1.2
    // distance <= radiusA - radiusB * 0.4
    // ------------------------------------------------------------

    const auto canEat =
        [](const Sim& attacker, const Target& victim) -> bool
        {
            if (attacker.mass <= victim.mass * 1.2f)
                return false;

            const float dx =
                victim.x - attacker.x;

            const float dy =
                victim.y - attacker.y;

            const float distance =
                std::sqrt(
                    dx * dx +
                    dy * dy
                );

            return
                distance <=
                attacker.radius -
                victim.radius * 0.4f;
        };

    const auto canBeEatenBy =
        [](const Sim& victim, const Target& attacker) -> bool
        {
            if (attacker.mass <= victim.mass * 1.2f)
                return false;

            const float dx =
                victim.x - attacker.x;

            const float dy =
                victim.y - attacker.y;

            const float distance =
                std::sqrt(
                    dx * dx +
                    dy * dy
                );

            return
                distance <=
                attacker.radius -
                victim.radius * 0.4f;
        };

    // ------------------------------------------------------------
    // 16 prediction ticks.
    // ------------------------------------------------------------

    for (int tick = 0;
        tick < predictionTicks;
        ++tick)
    {
        // ========================================================
        // 1. Обычное движение + расталкивание
        // ========================================================

        for (std::size_t aIndex = 0;
            aIndex < cells.size();
            ++aIndex)
        {
            Sim& a =
                cells[aIndex];

            if (!a.alive)
                continue;

            const float oldAX =
                a.x;

            const float oldAY =
                a.y;

            if (!stepToward(a))
                continue;

            const float newAX =
                a.x;

            const float newAY =
                a.y;

            for (std::size_t bIndex = 0;
                bIndex < cells.size();
                ++bIndex)
            {
                if (aIndex == bIndex)
                    continue;

                Sim& b =
                    cells[bIndex];

                if (!b.alive)
                    continue;

                if (
                    a.cooldown <= 0 &&
                    b.cooldown <= 0
                    )
                {
                    continue;
                }

                const float ox =
                    b.x - oldAX;

                const float oy =
                    b.y - oldAY;

                const float distanceSquared =
                    ox * ox +
                    oy * oy;

                const float radiusSum =
                    a.radius +
                    b.radius;

                if (
                    distanceSquared >=
                    radiusSum * radiusSum
                    )
                {
                    continue;
                }

                const float distance =
                    std::sqrt(
                        distanceSquared
                    );

                float nx =
                    b.x - newAX;

                float ny =
                    b.y - newAY;

                const float normalLength =
                    std::sqrt(
                        nx * nx +
                        ny * ny
                    );

                const float push =
                    radiusSum -
                    distance +
                    5.0f;

                if (normalLength > 0.000001f)
                {
                    b.x +=
                        nx / normalLength *
                        push;

                    b.y +=
                        ny / normalLength *
                        push;
                }
                else
                {
                    b.y +=
                        push;
                }
            }
        }

        // ========================================================
        // 2. Split impulse
        // ========================================================

        for (Sim& cell : cells)
        {
            if (!cell.alive)
                continue;

            if (cell.boost <= 0)
                continue;

            cell.x +=
                cell.vx;

            cell.y +=
                cell.vy;

            cell.vx *=
                impulseDecay;

            cell.vy *=
                impulseDecay;

            --cell.boost;
        }

        // ========================================================
// 3. Столкновения:
//    - наша клетка может съесть чужую;
//    - чужая может съесть нашу;
//    - наша клетка может лопнуть о вирус.
// ========================================================

        for (Sim& predicted : cells)
        {
            if (!predicted.alive)
                continue;

            if (!predicted.predicted)
                continue;

            for (Target& target : targets)
            {
                if (!target.alive)
                    continue;

                // ------------------------------------------------
                // Вирус.
                // ------------------------------------------------

                if (target.virus)
                {
                    if (canEat(
                        predicted,
                        target
                    ))
                    {
                        predicted.hitsVirus = true;
                        predicted.alive = false;
                    }

                    continue;
                }

                // ------------------------------------------------
                // Чужая клетка может съесть нашу.
                // ------------------------------------------------

                if (canBeEatenBy(
                    predicted,
                    target
                ))
                {
                    predicted.dangerLevel =
                        std::max(
                            predicted.dangerLevel,
                            3
                        );

                    predicted.alive = false;

                    continue;
                }

                // ------------------------------------------------
                // Наша клетка может съесть чужую.
                // ------------------------------------------------

                if (canEat(
                    predicted,
                    target
                ))
                {
                    predicted.dangerLevel =
                        std::max(
                            predicted.dangerLevel,
                            2
                        );

                    target.alive = false;

                    continue;
                }

                // ------------------------------------------------
                // Простое пересечение.
                // ------------------------------------------------

                const float dx =
                    target.x -
                    predicted.x;

                const float dy =
                    target.y -
                    predicted.y;

                const float distance =
                    std::sqrt(
                        dx * dx +
                        dy * dy
                    );

                if (
                    distance <
                    predicted.radius +
                    target.radius
                    )
                {
                    predicted.dangerLevel =
                        std::max(
                            predicted.dangerLevel,
                            1
                        );
                }
            }
        }

        // ========================================================
        // 4. Cooldown
        // ========================================================

        for (Sim& predicted : cells)
        {
            if (!predicted.alive)
                continue;

            if (!predicted.predicted)
                continue;

            for (const Target& target : targets)
            {
                if (target.virus)
                {
                    if (canEat(predicted, target))
                    {
                        predicted.hitsVirus = true;
                    }

                    continue;
                }

                // ------------------------------------------------
                // Уровень 3:
                // чужая клетка может съесть нашу.
                // ------------------------------------------------

                if (canBeEatenBy(predicted, target))
                {
                    predicted.dangerLevel =
                        std::max(
                            predicted.dangerLevel,
                            3
                        );

                    continue;
                }

                // ------------------------------------------------
                // Уровень 2:
                // наша клетка может съесть чужую.
                // ------------------------------------------------

                if (canEat(predicted, target))
                {
                    predicted.dangerLevel =
                        std::max(
                            predicted.dangerLevel,
                            2
                        );

                    continue;
                }

                // ------------------------------------------------
                // Уровень 1:
                // простое пересечение.
                // ------------------------------------------------

                const float dx =
                    target.x -
                    predicted.x;

                const float dy =
                    target.y -
                    predicted.y;

                const float distance =
                    std::sqrt(
                        dx * dx +
                        dy * dy
                    );

                if (
                    distance <
                    predicted.radius +
                    target.radius
                    )
                {
                    predicted.dangerLevel =
                        std::max(
                            predicted.dangerLevel,
                            1
                        );
                }
            }
        }

        // ========================================================
        // 4. Cooldown
        // ========================================================

        for (Sim& cell : cells)
        {
            if (cell.cooldown > 0)
                --cell.cooldown;
        }
    }

    // ------------------------------------------------------------
// Определяем чужие клетки, которые наши predicted-клетки
// могут съесть после split.
//
// Проверяем траекторию:
//
// startX/startY -> x/y
//
// а не только конечную позицию.
// ------------------------------------------------------------

    for (const Target& target : targets)
    {
        if (target.virus)
            continue;

        int dangerLevel = 0;

        for (const Sim& predicted : cells)
        {
            if (!predicted.predicted)
                continue;

            if (!predicted.alive &&
                !predicted.hitsVirus)
            {
                continue;
            }

            // --------------------------------------------------------
            // Уровень 2:
            // наша predicted-клетка может съесть enemy.
            //
            // Масса должна быть достаточной.
            // Равенство допустимо.
            // --------------------------------------------------------

            if (predicted.mass >= target.mass * 1.2f)
            {
                const float distance =
                    distToSegment(
                        target.x,
                        target.y,
                        predicted.startX,
                        predicted.startY,
                        predicted.x,
                        predicted.y
                    );

                if (distance <=
                    predicted.radius -
                    target.radius * 0.4f)
                {
                    dangerLevel =
                        std::max(
                            dangerLevel,
                            2
                        );
                }
            }

            // --------------------------------------------------------
            // Уровень 3:
            // enemy может съесть нашу predicted-клетку.
            //
            // Пока оставляем существующую механику.
            // --------------------------------------------------------

            if (target.mass >= predicted.mass * 1.2f)
            {
                const float distance =
                    distToSegment(
                        target.x,
                        target.y,
                        predicted.startX,
                        predicted.startY,
                        predicted.x,
                        predicted.y
                    );

                if (distance <=
                    target.radius -
                    predicted.radius * 0.4f)
                {
                    dangerLevel =
                        std::max(
                            dangerLevel,
                            3
                        );
                }
            }

            // --------------------------------------------------------
            // Уровень 1:
            // простое пересечение траектории.
            // --------------------------------------------------------

            const float distance =
                distToSegment(
                    target.x,
                    target.y,
                    predicted.startX,
                    predicted.startY,
                    predicted.x,
                    predicted.y
                );

            if (distance <
                predicted.radius +
                target.radius)
            {
                dangerLevel =
                    std::max(
                        dangerLevel,
                        1
                    );
            }
        }

        if (dangerLevel == 0)
            continue;

        SplitPredictionEnemy enemy;

        enemy.id =
            target.id;

        enemy.dangerLevel =
            dangerLevel;

        result.enemies.push_back(
            enemy
        );
    }


    // ------------------------------------------------------------
    // Формируем результат.
    // ------------------------------------------------------------

    result.cells.reserve(
        cells.size()
    );

    for (const Sim& cell : cells)
    {
        if (!cell.alive &&
            !cell.hitsVirus)
        {
            continue;
        }

        SplitPredictionCell predicted;

        predicted.x =
            cell.x;

        predicted.y =
            cell.y;

        predicted.radius =
            cell.radius;

        predicted.mass =
            cell.mass;

        predicted.dangerLevel =
            cell.dangerLevel;

        predicted.hitsVirus =
            cell.hitsVirus;

        result.cells.push_back(
            predicted
        );
    }

    return result;
}