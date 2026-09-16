#include "GameUpdater.h"

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
    float deltaTime,
    float screenW,
    float screenH,
    bool menuOpen
)
{
    skinManager.processCompleted();

    network.update();

    if (!gameState.mapCentered)
    {
        World::MapBounds bounds =
            world.getMapBounds();

        if (bounds.valid)
        {
            float minX =
                static_cast<float>(bounds.minX);

            float minY =
                static_cast<float>(bounds.minY);

            float maxX =
                static_cast<float>(bounds.maxX);

            float maxY =
                static_cast<float>(bounds.maxY);

            float middleX =
                (minX + maxX) * 0.5f;

            float middleY =
                (minY + maxY) * 0.5f;

            camera.setBounds(
                minX,
                minY,
                maxX,
                maxY
            );

            camera.setManualTarget(
                middleX,
                middleY
            );

            camera.x = middleX;
            camera.y = middleY;

            camera.setZoomImmediate(0.0621f);

            gameState.mapCentered = true;
        }
    }

    if (!menuOpen && input.mouseWheel != 0.0f)
    {
        camera.zoomBy(input.mouseWheel);
    }

    if (!menuOpen && input.leftButtonJustPressed)
    {
        float worldX;
        float worldY;

        camera.screenToWorld(
            input.mouseX,
            input.mouseY,
            screenW,
            screenH,
            worldX,
            worldY
        );

        camera.setManualTarget(
            worldX,
            worldY
        );

        network.sendAimPosition(
            worldX,
            worldY
        );
    }

    if (!ownedIds.empty())
    {
        camera.setZoomLimits(
            0.2f,
            6.0f,
            0.05f,
            2.0f
        );

        camera.snapTowardsTarget(
            deltaTime,
            0.01667f
        );

        camera.updateZoomOnly(deltaTime);
    }
    else
    {
        camera.setZoomLimits(
            0.2f,
            1.5f,
            0.05f,
            0.4f
        );

        camera.update(deltaTime);
    }
}