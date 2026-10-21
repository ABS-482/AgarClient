#pragma once
// VirusEjector — набросок. Единицы: мир — юниты, время — секунды, скорость — юниты/с.
// Заменить под свой клиент: Vec2/Body, источники позиций, sendMove/sendEject, часы.

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <functional>
#include <iterator>
#include <limits>
#include <optional>
#include <unordered_map>
#include <utility>
#include <vector>

namespace virus_ejector {

    // ---------------------------------------------------------------- math
    struct Vec2 { float x = 0.f, y = 0.f; };
    inline Vec2  operator+(Vec2 a, Vec2 b) { return { a.x + b.x, a.y + b.y }; }
    inline Vec2  operator-(Vec2 a, Vec2 b) { return { a.x - b.x, a.y - b.y }; }
    inline Vec2  operator*(Vec2 a, float k) { return { a.x * k, a.y * k }; }
    inline float dot(Vec2 a, Vec2 b) { return a.x * b.x + a.y * b.y; }
    inline float len(Vec2 a) { return std::sqrt(dot(a, a)); }
    inline double posmod(double x, double m) { x = std::fmod(x, m); return x < 0 ? x + m : x; }

    struct Body {
        uint32_t id = 0;
        Vec2 pos;
        float radius = 0.f;
        // Время последнего измерения координат на часах steady_clock, секунды.
        // Отрицательное значение означает использовать snapshotTime из onSnapshot().
        double observedAt = -1.0;
    };

    // ---------------------------------------------------------------- скорость по снимкам
    // Как в b/d.java: V = (P1 - P0) / dt, dt = разница времён снимков.
    // Сверху EMA, чтобы шум dt не раздувал V и упреждение.
    class MotionTracker {
    public:
        explicit MotionTracker(float alpha = 0.6f, double maxGap = 0.5)
            : alpha_(alpha), maxGap_(maxGap) {
        }

        void begin() { ++epoch_; }
        void end() {
            for (auto it = tracks_.begin(); it != tracks_.end();)
                it = (it->second.seen != epoch_) ? tracks_.erase(it) : std::next(it);
        }

        void observe(uint32_t id, Vec2 pos, double t) {
            auto [it, inserted] = tracks_.try_emplace(id);
            Track& tr = it->second;
            tr.seen = epoch_;
            if (inserted) { tr.pos = pos; tr.t = t; return; }

            const double dt = t - tr.t;
            if (dt <= 1e-3) return;                       // тот же снимок
            if (dt > maxGap_) { tr.hasVel = false; tr.vel = {}; }
            else {
                const Vec2 raw = (pos - tr.pos) * float(1.0 / dt);
                tr.vel = tr.hasVel ? raw * alpha_ + tr.vel * (1.f - alpha_) : raw;
                tr.hasVel = true;
            }
            tr.pos = pos; tr.t = t;
        }

        // Не используем скорость, если конкретная сущность давно не обновлялась.
        bool velocity(uint32_t id, Vec2& out, double now) const {
            auto it = tracks_.find(id);
            if (it == tracks_.end() || !it->second.hasVel) return false;
            if (now < it->second.t || now - it->second.t > maxGap_) return false;
            out = it->second.vel;
            return true;
        }

    private:
        struct Track { Vec2 pos, vel; double t = 0; bool hasVel = false; uint64_t seen = 0; };
        std::unordered_map<uint32_t, Track> tracks_;
        float alpha_; double maxGap_; uint64_t epoch_ = 0;
    };

    // ---------------------------------------------------------------- настройки
    struct Config {
        float tick = 0.050f;  // тик сервера
        float shotInterval = 0.150f;  // минимум между eject (3 тика)
        float holdMargin = 0.010f;  // запас на джиттер при удержании курсора

        float ejectSpeed = 0.f;     // юнит/с полёта выброса; 0 = не учитывать полёт (ПОДОБРАТЬ)
        float virusDecay = 1.f;     // множитель скорости вируса за тик; 1 = линейно (ПОДОБРАТЬ)

        float minVirusSpeed = 1.f;     // ниже — вирус считаем стоящим
        float maxTTI = 2.f;     // дальше этого времени не реагируем
        float massRatio = 1.2f;    // правило поедания (как в клиенте): Rc² >= 1.2·Rv²
        float hitRadiusK = 0.4f;    //                                   d <= Rc − 0.4·Rv
        float hitMargin = 0.f;     // запас к радиусу попадания

        // На первом этапе оставляем false: рассчитываем угрозы, но ничего не отправляем.
        // Включать только после интеграции приоритета aim и ручного ввода в main.cpp.
        bool autoFire = false;
    };

    struct Threat {
        uint32_t virusId = 0, cellId = 0;
        Vec2  aim;          // куда ставить курсор
        float tti = 0;      // время до попадания вируса в клетку, от момента снимка
        float tau = 0;      // горизонт упреждения до момента попадания выброса, от момента снимка
    };

    // ---------------------------------------------------------------- основной класс
    class VirusEjector {
    public:
        using MoveFn = std::function<void(Vec2 world)>;
        using EjectFn = std::function<void()>;

        VirusEjector(Config cfg, MoveFn sendMove, EjectFn sendEject)
            : cfg_(cfg), sendMove_(std::move(sendMove)), sendEject_(std::move(sendEject)) {
        }

        // Передавать полный текущий набор наблюдаемых сущностей при каждом обновлении.
        // snapshotTime — резервная временная отметка для Body без observedAt.
        // arrivalTime — время прихода последнего изменившегося World snapshot на steady_clock.
        // observedAt задаёт время координат конкретной сущности; для собственных клеток
        // можно передавать текущую render-позицию с observedAt = now.
        void onSnapshot(double snapshotTime, double arrivalTime,
            std::vector<Body> viruses, std::vector<Body> ownCells) {
            virusTracks_.begin(); cellTracks_.begin();
            for (const Body& v : viruses)
                virusTracks_.observe(
                    v.id,
                    v.pos,
                    v.observedAt >= 0.0 ? v.observedAt : snapshotTime
                );

            for (const Body& c : ownCells)
                cellTracks_.observe(
                    c.id,
                    c.pos,
                    c.observedAt >= 0.0 ? c.observedAt : snapshotTime
                );
            virusTracks_.end(); cellTracks_.end();
            viruses_ = std::move(viruses);
            cells_ = std::move(ownCells);
            lastArrival_ = arrivalTime;
            havePhase_ = true;
        }

        // Всегда вызывать при движении мыши пользователем.
        void onUserCursor(Vec2 world) { userCursor_ = world; }
        // В фазе Hold курсор пользователя на сервер не пересылать (иначе перебьёт aim).
        bool shouldForwardUserCursor() const { return phase_ == Phase::Idle; }

        // Вызывать каждый кадр. now и rtt — секунды на шкале steady_clock.
        // По умолчанию метод только обновляет latestThreat(); сетевые команды не отправляются.
        void update(double now, double rtt) {
            if (cfg_.autoFire && phase_ == Phase::Hold) {
                if (now >= restoreAt_) {
                    sendMove_(userCursor_); // вернуть курсор после тика обработки eject
                    phase_ = Phase::Idle;
                }
                latestThreat_.reset();
                return;
            }

            latestThreat_ = findThreat(now, rtt);

            // Диагностический режим — только прогноз, без управления и выброса массы.
            if (!cfg_.autoFire || !latestThreat_)
                return;

            if (now - lastShot_ < cfg_.shotInterval)
                return;

            sendMove_(latestThreat_->aim); // 1) курсор
            sendEject_();                  // 2) eject в том же цикле
            lastShot_ = now;
            restoreAt_ = now + tickWait(now, rtt) + cfg_.holdMargin;
            phase_ = Phase::Hold;
        }

        const std::optional<Threat>& latestThreat() const {
            return latestThreat_;
        }

        // ------------------------------------------------------------ формулы
        // Сколько eject будет ждать обработки сервером после отправки сейчас.
        // Снимок сгенерирован на сервере в g = arrival - rtt/2, границы тиков: g + k·tick.
        // Пакет придёт в now + rtt/2, обработка на ближайшей границе:
        //   wait = posmod(arrival - now - rtt, tick)
        // Без фазы — худший случай: целый тик. Для стабильности arrival лучше сглаживать по окружности (mod tick).
        float tickWait(double now, double rtt) const {
            if (!havePhase_) return cfg_.tick;
            return float(posmod(lastArrival_ - now - rtt, cfg_.tick));
        }

        // Смещение за время tau при затухании скорости на virusDecay за тик:
        //   сумма V·tick·ρ^i, i = 0..n-1,  n = tau/tick  =>  V·tick·(1-ρ^n)/(1-ρ)
        Vec2 advance(Vec2 pos, Vec2 vel, float tau) const {
            const float rho = cfg_.virusDecay;
            if (tau <= 0.f)
                return pos;
            if (cfg_.tick <= 0.f || rho >= 0.9999f)
                return pos + vel * tau;
            if (rho <= 0.f)
                return pos + vel * std::min(tau, cfg_.tick);

            const float n = tau / cfg_.tick;
            return pos + vel * (
                cfg_.tick * (1.f - std::pow(rho, n)) / (1.f - rho)
                );
        }

        std::optional<Threat> findThreat(double now, double rtt) const {
            const float wait = tickWait(now, rtt);
            // Позиции клеток здесь текущие (render state), а позиции вирусов
            // могут быть старше: projected-to-now ниже учитывает их собственное время.
            const float tauProc = std::max(0.f, static_cast<float>(rtt)) + wait;

            std::optional<Threat> best;
            for (const Body& v : viruses_) {
                Vec2 vv;
                if (!virusTracks_.velocity(v.id, vv, now) || len(vv) < cfg_.minVirusSpeed)
                    continue;

                const double observedAt = v.observedAt >= 0.0 ? v.observedAt : lastArrival_;
                if (now < observedAt)
                    continue;

                const float virusAge = static_cast<float>(now - observedAt);
                // Приводим вирус к текущему времени, прежде чем считать TTI.
                const Vec2 virusNow = advance(v.pos, vv, virusAge);

                for (const Body& c : cells_) {
                    // 1) Проверка, может ли клетка съесть вирус.
                    if (c.radius * c.radius < cfg_.massRatio * v.radius * v.radius)
                        continue;

                    const float rHit = c.radius - cfg_.hitRadiusK * v.radius + cfg_.hitMargin;
                    if (rHit <= 0.f)
                        continue;

                    // 2) Время до попадания относительно текущих позиций:
                    // |q + w*t| = rHit, где w — относительная скорость.
                    Vec2 vc{};
                    cellTracks_.velocity(c.id, vc, now); // если скорость недоступна — считаем клетку неподвижной
                    const Vec2 q = virusNow - c.pos;
                    const Vec2 w = vv - vc;
                    const float a = dot(w, w);
                    if (a < 1e-6f)
                        continue;

                    const float b = dot(q, w);
                    const float cc = dot(q, q) - rHit * rHit;
                    const float disc = b * b - a * cc;
                    if (disc < 0.f)
                        continue;

                    const float root = std::sqrt(disc);
                    const float t1 = (-b - root) / a;
                    const float t2 = (-b + root) / a;
                    if (t2 < 0.f)
                        continue;

                    const float tti = std::max(0.f, t1);
                    if (tti > cfg_.maxTTI)
                        continue;

                    // 3) Прицел в прогнозируемую позицию вируса при обработке выстрела.
                    const Vec2 origin = c.pos + vc * tauProc;
                    float tau = tauProc;
                    Vec2 aim = advance(virusNow, vv, tau);
                    for (int i = 0; i < 3; ++i) {
                        aim = advance(virusNow, vv, tau);
                        const float flight = cfg_.ejectSpeed > 0.f
                            ? std::max(0.f, len(aim - origin) - c.radius) / cfg_.ejectSpeed
                            : 0.f;
                        tau = tauProc + flight;
                    }
                    aim = advance(virusNow, vv, tau);

                    // 4) Если выстрел не успеет до столкновения, угрозу игнорируем.
                    if (tti <= tau)
                        continue;

                    if (!best || tti < best->tti)
                        best = Threat{ v.id, c.id, aim, tti, tau };
                }
            }
            return best; // ближайшее по времени столкновение
        }

    private:
        enum class Phase { Idle, Hold };
        Config cfg_;
        MoveFn sendMove_; EjectFn sendEject_;
        MotionTracker virusTracks_, cellTracks_;
        std::vector<Body> viruses_, cells_;
        std::optional<Threat> latestThreat_;
        Vec2 userCursor_{};
        Phase phase_ = Phase::Idle;
        double lastShot_ = -1e9, restoreAt_ = 0, lastArrival_ = 0;
        bool havePhase_ = false;
    };

}  // namespace virus_ejector