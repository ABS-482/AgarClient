#include "GameUpdater.h"

#include <algorithm>

GameUpdater::GameUpdater(
    Camera& camera,
    NetworkClient& network,
    SkinManager& skinManager,
    World& world,
    GameState& gameState
)
    : camera(camera)
    , network(network)
    , skinManager(skinManager)
    , world(world)
    , gameState(gameState)
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

    // --------------------------------------------------------
    // Границы карты.
    // --------------------------------------------------------

    const World::MapBounds bounds = world.getMapBounds();

    CameraInput cameraInput;

    if (bounds.valid)
    {
        // Camera использует правую и верхнюю границы мира
        // для ограничения положения камеры.
        cameraInput.worldSize = {
            static_cast<float>(bounds.maxX),
            static_cast<float>(bounds.maxY)
        };

        // Первичное центрирование на карте.
        // Java GameScreen.setCameraPos() сразу устанавливает
        // позицию камеры, не подбирая zoom для всей карты.
        if (!gameState.mapCentered)
        {
            const float centerX =
                static_cast<float>((bounds.minX + bounds.maxX) * 0.5);

            const float centerY =
                static_cast<float>((bounds.minY + bounds.maxY) * 0.5);

            camera.setCameraPos(cameraInput, centerX, centerY);

            spectatorTarget = { centerX, centerY };
            spectatorTargetInitialized = true;

            gameState.mapCentered = true;
        }
    }

    // --------------------------------------------------------
    // Размер окна и настройки камеры.
    // --------------------------------------------------------

    cameraInput.smallInterface = camera.smallInterface;
    cameraInput.mode = gameState.cameraMode;
    cameraInput.pixelsEnabled = gameState.pixelsEnabled;
    cameraInput.drawingMode = gameState.drawingMode;

    cameraInput.hasKiller = gameState.hasKiller;
    cameraInput.killerPos = gameState.killerPos;
    cameraInput.killerRadius = gameState.killerRadius;

    // --------------------------------------------------------
    // Начальное значение цели наблюдателя.
    // --------------------------------------------------------

    if (!spectatorTargetInitialized && bounds.valid)
    {
        spectatorTarget = {
            static_cast<float>((bounds.minX + bounds.maxX) * 0.5),
            static_cast<float>((bounds.minY + bounds.maxY) * 0.5)
        };

        spectatorTargetInitialized = true;
    }

    // --------------------------------------------------------
    // Колесо мыши.
    //
    // onScroll() принимает величину прокрутки в единицах
    // InputState, а не браузерное значение zoomBy().
    // --------------------------------------------------------

    if (!menuOpen && input.mouseWheel != 0.0f)
    {
        camera.onScroll(
            input.mouseWheel,
            cameraInput.pixelsEnabled
        );
    }

    // --------------------------------------------------------
    // Клик в режиме наблюдателя.
    // --------------------------------------------------------

    if (!menuOpen &&
        camera.spectator &&
        input.leftButtonJustPressed)
    {
        float worldX = 0.0f;
        float worldY = 0.0f;

        camera.screenToWorld(
            input.mouseX,
            input.mouseY,
            worldX,
            worldY
        );

        spectatorTarget = { worldX, worldY };
        spectatorTargetInitialized = true;

        network.sendAimPosition(worldX, worldY);
    }

    cameraInput.inputTarget = spectatorTarget;

    // --------------------------------------------------------
    // Собственные клетки.
    //
    // Java сначала вызывает Blob.act(dt), потом Player.act(dt)
    // накапливает координаты уже обновлённых клеток.
    //
    // Здесь используем отображаемые RenderState, которые уже
    // обновлены в игровом цикле текущего кадра.
    // --------------------------------------------------------

    cameraCells.clear();

    const WorldSnapshot snapshot = world.snapshot();

    for (const uint32_t id : ownedIds)
    {
        const auto renderIt = renderStates.find(id);

        if (renderIt == renderStates.end() ||
            !renderIt->second.initialized)
        {
            continue;
        }

        const RenderState& rs = renderIt->second;

        cameraCells.push_back({
            rs.x,
            rs.y,
            rs.size
            });

        // В Java Player.act() обновляет headPos/headRadius
        // для клетки, у которой isAgitated() == true.
        // В разобранном сетевом коде этот признак соответствует
        // биту 0x10 в flags.
        const auto blobIt = snapshot->find(id);

        if (blobIt != snapshot->end() &&
            (blobIt->second.flags & 0x10) != 0)
        {
            trackedHeadPos = { rs.x, rs.y };
            trackedHeadRadius = rs.size;
        }
    }

    // fromCells вычисляет:
    // blobsCount, averagePos и totalRadius.
    cameraInput.fromCells(cameraCells);

    cameraInput.headPos = trackedHeadPos;
    cameraInput.headRadius = trackedHeadRadius;

    // --------------------------------------------------------
    // Обновление камеры по формулам Camera::update().
    // --------------------------------------------------------

    Vec2f newTarget{};

    if (camera.update(deltaTime, cameraInput, &newTarget))
    {
        spectatorTarget = newTarget;
        spectatorTargetInitialized = true;
    }
}