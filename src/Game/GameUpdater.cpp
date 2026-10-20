#include "GameUpdater.h"

#include <algorithm>

GameUpdater::GameUpdater(
    Camera& camera,
    NetworkClient& network,
    SkinManager& skinManager,
    World& world,
    GameState& gameState
)
    : camera(camera),
    network(network),
    skinManager(skinManager),
    world(world),
    gameState(gameState)
{
}

void GameUpdater::update(
    const InputState& input,
    const std::vector<uint32_t>& ownedIds,
    const std::unordered_map<uint32_t, RenderState>& renderStates,
    float deltaTime,
    float screenW,
    float screenH,
    bool menuOpen
)
{
    // JS: onResize()
    camera.onResize(screenW, screenH);

    // Пока своих клеток нет, камера свободная (в JS это isSpectating).
    camera.isSpectating = ownedIds.empty();

    // --- Начальное центрирование на карте ---
    if (!gameState.mapCentered)
    {
        const World::MapBounds bounds = world.getMapBounds();

        if (bounds.valid)
        {
            const float minX = static_cast<float>(bounds.minX);
            const float minY = static_cast<float>(bounds.minY);
            const float maxX = static_cast<float>(bounds.maxX);
            const float maxY = static_cast<float>(bounds.maxY);

            const float cx = (minX + maxX) * 0.5f;
            const float cy = (minY + maxY) * 0.5f;

            // Камера сразу стоит в центре карты, без сглаживания.
            camera.middleX = camera.offsetX = cx;
            camera.middleY = camera.offsetY = cy;

            // Масштаб, при котором вся карта помещается в окно.
            const float mapSize = std::max(maxX - minX, maxY - minY);
            const float fit = std::min(screenW, screenH) / mapSize;
            camera.ratio = camera.chunk = fit;

            // Подбираем zoom так, чтобы unitRatio() == 1: тогда мёртвая ветка
            // update() (ratio -> chunk * unitRatio()) держит ratio на fit.
            camera.zoom = 1.0f / std::max(screenH / 1080.0f, screenW / 1920.0f);

            gameState.mapCentered = true;
        }
    }

    // --- Колесо мыши ---
    if (!menuOpen && input.mouseWheel != 0.0f)
    {
        // Camera::zoomBy ждёт браузерные единицы (±120 за щелчок).
        // Если input.mouseWheel уже в таких единицах, убери "* 120.0f".
        camera.zoomBy(input.mouseWheel * 120.0f);
    }

    // --- Клик в режиме наблюдателя: переносим камеру в точку ---
    if (!menuOpen && input.leftButtonJustPressed && ownedIds.empty())
    {
        float worldX = 0.0f;
        float worldY = 0.0f;

        camera.screenToWorld(input.mouseX, input.mouseY, worldX, worldY);

        // offset сам плавно доедет до middle (формула 29/30 из JS).
        camera.middleX = worldX;
        camera.middleY = worldY;

        network.sendAimPosition(worldX, worldY);
    }

    // --- Клетки игрока для камеры ---
    cameraCells.clear();

    if (!ownedIds.empty())
    {
        for (uint32_t id : ownedIds)
        {
            const auto it = renderStates.find(id);
            if (it == renderStates.end() || !it->second.initialized)
                continue;

            const RenderState& rs = it->second;
            cameraCells.push_back({ rs.x, rs.y, rs.size });
        }
    }

    camera.update(cameraCells, deltaTime);
}