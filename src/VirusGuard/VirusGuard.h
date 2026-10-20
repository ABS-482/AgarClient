#pragma once

#include <chrono>
#include <cstdint>
#include <unordered_map>
#include <limits>

class World;

class VirusGuard
{
public:
    explicit VirusGuard(World& world);

    void update(
        float cursorX,
        float cursorY
    );

    bool solve();

    bool hasTarget() const;

    bool planShot();

    float shotX() const;
    float shotY() const;

    bool shotIsFeed() const;
    bool shotEjects() const;

    uint32_t targetId() const;

    float targetX() const;
    float targetY() const;

    float targetInterceptX() const;
    float targetInterceptY() const;
    void noteOwnEject();
    void noteGuardFeed();
    void noteShotFired();

    float targetDangerTime() const;

    float distanceToNearestCell(float x, float y) const;

    float feedRange() const;
    float trackRange() const;
    float shotCost() const;
    bool virusNearby() const;

    bool targetPredictedPosition(
        float time,
        float& outX,
        float& outY
    ) const;

    struct Config
    {
        float maxVirusSpeed = 2500.0f;
        float velEma = 0.4f;
        float velHold = 0.35f;
        float minStepDist = 14.0f;
        float minFlySpeed = 120.0f;

        float virusDecelTau = 2.5f;

        float pelletMax = 1300.0f;
        float pelletTau = 0.45f;

        float lookahead = 3.0f;
        float dangerMargin = 130.0f;
        float standingPauseMs = 2200.0f;
        float minReach = 500.0f;
        float maxReach = 2200.0f;

        float minOwnMass = 200.0f;
        float aimBeyond = 320.0f;
        float approachMargin = 400.0f;

        float armedMass = 160.0f;
        float armedFeedFresh = 0.35f;
        float minFeeds = 3.0f;
        float feedForget = 1.5f;
        float feederRange = 2500.0f;

        float selfFeedWindow = 1.2f;
        int armedShots = 2;
        int ownFeedImmunityMs = 3000;
        float massBudgetShare = 0.15f;
        float massBudgetFloor = 600.0f;
        float ejectMass = 13.0f;
        float recentInboundSec = 1.1f;
        int flyingBurst = 16;
        int ejectsPerShot = 2;
        int reengageMs = 60;
        float fireMass = 174.0f;
        float feedStep = 13.0f;
        bool learnEjectSpeed = false;
        float ejectSpeed = 1400.0f;
        bool armedMode = false;
        bool stayInPlace = true;
        bool freezeOnlyWhileEngaging = true;
        float stayRadius = 3500.0f;
        int stayPoints = 4;
        int feedTickMs = 38;
        int shotIntervalMs = 30;
    };

    const Config& config() const
    {
        return m_config;
    }

    float aimX() const;
    float aimY() const;

private:
    struct OwnCell
    {
        uint32_t id = 0;

        float x = 0.0f;
        float y = 0.0f;
        float radius = 0.0f;
    };

    struct EnemyCell
    {
        uint32_t id = 0;

        float x = 0.0f;
        float y = 0.0f;
        float radius = 0.0f;
    };

    struct VirusTrack
    {
        uint32_t id = 0;

        float x = 0.0f;
        float y = 0.0f;
        float radius = 0.0f;

        // Состояние вируса
        float mass = 0.0f;
        float feeds = 0.0f;
        float feedAge = 0.0f;

        // Опорная точка для оценки скорости
        float refX = 0.0f;
        float refY = 0.0f;
        float refAge = 0.0f;

        float step = 0.0f;

        // В оригинале FLT_MAX означает "нет данных"
        float inbound = std::numeric_limits<float>::max();

        // Защита от засчёта собственной подкормки
        float selfAge = 0.0f;
        float ownFedAge = 0.0f;
        // Состояние стрельбы по этому вирусу
        int shots = 0;
        long long idleUntil = 0;

        // Уже достиг нашей клетки
        bool arrived = false;

        // Дробный счётчик подкормок
        float shotAcc = 0.0f;

        // Скорость
        float velocityX = 0.0f;
        float velocityY = 0.0f;
        bool hasVelocity = false;
        bool seenThisUpdate = false;

        // Рассчитанная точка перехвата
        float interceptX = 0.0f;
        float interceptY = 0.0f;
        bool interceptSolved = false;
    };

    World& m_world;

    Config m_config;

    std::unordered_map<uint32_t, VirusTrack> m_viruses;

    std::unordered_map<uint32_t, OwnCell> m_ownCells;
    std::unordered_map<uint32_t, EnemyCell> m_enemyCells;

    uint32_t m_targetId = 0;
    uint32_t m_stickyId = 0;

    float m_aimX = 0.0f;
    float m_aimY = 0.0f;

    float m_targetX = 0.0f;
    float m_targetY = 0.0f;

    float m_cursorX = 0.0f;
    float m_cursorY = 0.0f;

    float m_targetInterceptX = 0.0f;
    float m_targetInterceptY = 0.0f;

    float m_targetDangerTime =
        std::numeric_limits<float>::max();

    bool m_hasTarget = false;
    bool m_virusNearby = false;

    float virusTravel(
        const VirusTrack& virus,
        float speed,
        float time
    ) const;

    float dangerTime(
        const VirusTrack& virus
    ) const;

    float armedThreat(
        const VirusTrack& virus
    ) const;

    void updateVirusNearby();
    void markPlayerFed();
    float pelletReach() const;

    float ejectTravel(float time) const;

    const OwnCell* nearestOwnCell(
        float x,
        float y
    ) const;

    void intercept(
        VirusTrack& virus,
        float speed
    ) const;

    void aimThrough(
        VirusTrack& virus
    );

    void updateCenter();

    float budgetPerSec() const;

    bool canEject() const;

    bool isFlying(
        const VirusTrack& virus
    ) const;

    bool engagedAsFlying(
        const VirusTrack& virus
    ) const;

    int pelletsReaching(
        const VirusTrack& virus
    ) const;

    int neededEjects(
        const VirusTrack& virus
    ) const;

    float m_maxOwnRadius = 0.0f;
    float m_maxOwnMass = 0.0f;
    float m_totalOwnMass = 0.0f;
    float m_ownEjectAge = 0.0f;
    float m_guardFeedAge = 0.0f;
    float m_centerX = 0.0f;
    float m_centerY = 0.0f;
    float m_massSpent = 0.0f;
    float m_delta = 1.0f / 60.0f;
    float m_shotX = 0.0f;
    float m_shotY = 0.0f;

    bool m_shotIsFeed = false;
    bool m_shotEjects = false;
    bool m_lastShotWasFeed = false;
    int m_stayStep = 0;
    // Для аналога Java delta между кадрами.
    std::chrono::steady_clock::time_point m_lastUpdateTime =
        std::chrono::steady_clock::now();

    bool m_hasUpdateTime = false;
};