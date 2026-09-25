#include "GameRenderer.h"

#include "PlayerColors.h"

#include <algorithm>
#include <chrono>
#include <cmath>

GameRenderer::GameRenderer(
    Shader& circleShader,
    Shader& skinShader,
    Shader& circleInstancedShader,
    CircleMesh& circleMesh,
    SkinMesh& skinMesh,
    InstancedCircleRenderer& foodRenderer,
    SkinManager& skinManager,
    TextRenderer& textRenderer,
    Font& font,
    Camera& camera
)
    : circleShader(circleShader),
    skinShader(skinShader),
    circleInstancedShader(circleInstancedShader),
    circleMesh(circleMesh),
    skinMesh(skinMesh),
    foodRenderer(foodRenderer),
    skinManager(skinManager),
    textRenderer(textRenderer),
    font(font),
    camera(camera)
{
    uCenter =
        circleShader.uniformLocation("uCenter");

    uRadius =
        circleShader.uniformLocation("uRadius");

    uCameraPos =
        circleShader.uniformLocation("uCameraPos");

    uZoom =
        circleShader.uniformLocation("uZoom");

    uScreenSize =
        circleShader.uniformLocation("uScreenSize");

    uColor =
        circleShader.uniformLocation("uColor");


    skinCenter =
        skinShader.uniformLocation("uCenter");

    skinRadius =
        skinShader.uniformLocation("uRadius");

    skinCameraPos =
        skinShader.uniformLocation("uCameraPos");

    skinZoom =
        skinShader.uniformLocation("uZoom");

    skinScreenSize =
        skinShader.uniformLocation("uScreenSize");

    skinTexture =
        skinShader.uniformLocation("uSkin");


    iCameraPos =
        circleInstancedShader.uniformLocation("uCameraPos");

    iZoom =
        circleInstancedShader.uniformLocation("uZoom");

    iScreenSize =
        circleInstancedShader.uniformLocation("uScreenSize");

    uViewportSize =
        glGetUniformLocation(
            circleShader.id(),
            "uViewportSize"
        );

    skinViewportSize =
        glGetUniformLocation(
            skinShader.id(),
            "uViewportSize"
        );

    iViewportSize =
        glGetUniformLocation(
            circleInstancedShader.id(),
            "uViewportSize"
        );
}

void GameRenderer::updateRenderStates(
    const WorldSnapshot& blobs,
    std::unordered_map<uint32_t, RenderState>& renderStates
)
{
    constexpr float interpolationDuration = 0.12f;

    const auto now =
        std::chrono::steady_clock::now();

    for (const auto& [id, blob] : *blobs)
    {
        RenderState& rs =
            renderStates[id];

        if (!rs.initialized)
        {
            rs.prevX = rs.x = blob.targetX;
            rs.prevY = rs.y = blob.targetY;
            rs.prevSize = rs.size = blob.targetSize;

            rs.lastSeenUpdate =
                blob.lastUpdateTime;

            rs.lastCellType =
                blob.cellType;

            rs.initialized = true;
        }
        else if (blob.cellType != rs.lastCellType)
        {
            rs.prevX = rs.x = blob.targetX;
            rs.prevY = rs.y = blob.targetY;
            rs.prevSize = rs.size = blob.targetSize;

            rs.lastSeenUpdate =
                blob.lastUpdateTime;

            rs.lastCellType =
                blob.cellType;
        }
        else if (blob.lastUpdateTime != rs.lastSeenUpdate)
        {
            rs.prevX = rs.x;
            rs.prevY = rs.y;
            rs.prevSize = rs.size;

            rs.lastSeenUpdate =
                blob.lastUpdateTime;
        }

        const float elapsed =
            std::chrono::duration<float>(
                now - blob.lastUpdateTime
            ).count();

        const float t =
            std::clamp(
                elapsed / interpolationDuration,
                0.0f,
                1.0f
            );

        rs.x =
            rs.prevX +
            (blob.targetX - rs.prevX) * t;

        rs.y =
            rs.prevY +
            (blob.targetY - rs.prevY) * t;

        rs.size =
            rs.prevSize +
            (blob.targetSize - rs.prevSize) * t;
    }

    for (auto it = renderStates.begin();
        it != renderStates.end(); )
    {
        if (blobs->find(it->first) == blobs->end())
        {
            it = renderStates.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

void GameRenderer::draw(
    const WorldSnapshot& blobs,
    const std::vector<uint32_t>& ownedIds,
    std::unordered_map<uint32_t, RenderState>& renderStates,
    float screenW,
    float screenH
)
{
    glClearColor(
        19.0f / 255.0f,
        40.0f / 255.0f,
        71.0f / 255.0f,
        1.0f
    );

    glClear(GL_COLOR_BUFFER_BIT);

    const float viewportPixelScale =
        screenW / camera.viewportWidth;

    drawList.clear();
    drawList.reserve(blobs->size());

    foodInstanceData.clear();
    foodInstanceData.reserve(blobs->size() * 6);

    for (const auto& [id, blob] : *blobs)
    {
        RenderState& rs =
            renderStates[id];

        if (
            blob.cellType == CellType::Food ||
            blob.cellType == CellType::EjectedMass
            )
        {
            RGB rgb =
                getPlayerColor(blob.colorIndex);

            foodInstanceData.insert(
                foodInstanceData.end(),
                {
                    rs.x,
                    rs.y,
                    rs.size,
                    rgb.r / 255.0f,
                    rgb.g / 255.0f,
                    rgb.b / 255.0f
                }
            );
        }
        else
        {
            drawList.push_back(
                { id, &blob, &rs }
            );
        }
    }

    std::sort(
        drawList.begin(),
        drawList.end(),
        [](const DrawEntry& a, const DrawEntry& b)
        {
            return a.rs->size < b.rs->size;
        }
    );
    // Настройка circle shader
    circleShader.use();

    circleShader.setVec2(
        uCameraPos,
        camera.x,
        camera.y
    );

    circleShader.setFloat(
        uZoom,
        camera.zoom
    );

    circleShader.setVec2(
        uScreenSize,
        screenW,
        screenH
    );

    circleShader.setVec2(
        uViewportSize,
        camera.viewportWidth,
        camera.viewportHeight
    );


    // Настройка skin shader
    skinShader.use();

    skinShader.setVec2(
        skinCameraPos,
        camera.x,
        camera.y
    );

    skinShader.setFloat(
        skinZoom,
        camera.zoom
    );

    skinShader.setVec2(
        skinScreenSize,
        screenW,
        screenH
    );

    skinShader.setVec2(
        skinViewportSize,
        camera.viewportWidth,
        camera.viewportHeight
    );


    // Настройка instanced food shader
    circleInstancedShader.use();

    circleInstancedShader.setVec2(
        iCameraPos,
        camera.x,
        camera.y
    );

    circleInstancedShader.setFloat(
        iZoom,
        camera.zoom
    );

    circleInstancedShader.setVec2(
        iScreenSize,
        screenW,
        screenH
    );

    circleInstancedShader.setVec2(
        iViewportSize,
        camera.viewportWidth,
        camera.viewportHeight
    );

    foodRenderer.draw(
        foodInstanceData.data(),
        foodInstanceData.size() / 6
    );

    constexpr float virusFontMultiplier = 1.6f;

    textRenderer.begin();

    for (const auto& entry : drawList)
    {
        const Blob& blob =
            *entry.blob;

        RenderState& rs =
            *entry.rs;

        // Круг
        circleShader.use();

        circleShader.setVec2(
            uCenter,
            rs.x,
            rs.y
        );

        circleShader.setFloat(
            uRadius,
            rs.size
        );

        if (blob.cellType == CellType::Virus)
        {
            circleShader.setVec3(
                uColor,
                1.0f,
                153.0f / 255.0f,
                0.0f
            );
        }
        else
        {
            RGB rgb =
                getPlayerColor(blob.colorIndex);

            circleShader.setVec3(
                uColor,
                rgb.r / 255.0f,
                rgb.g / 255.0f,
                rgb.b / 255.0f
            );
        }

        circleMesh.draw();

        // Скин
        const bool wantsSkin =
            (
                blob.cellType == CellType::Player ||
                blob.cellType == CellType::Virus
                ) &&
            blob.skin != 0;

        if (wantsSkin)
        {
            GLuint texture =
                skinManager.getTexture(blob.skin);

            if (texture != 0)
            {
                skinShader.use();

                skinShader.setVec2(
                    skinCenter,
                    rs.x,
                    rs.y
                );

                skinShader.setFloat(
                    skinRadius,
                    rs.size
                );

                glActiveTexture(GL_TEXTURE0);
                glBindTexture(
                    GL_TEXTURE_2D,
                    texture
                );

                skinShader.setInt(
                    skinTexture,
                    0
                );

                skinMesh.draw();
            }
        }

        int score = 0;
        bool showText = true;

        if (
            blob.cellType == CellType::Player ||
            blob.cellType == CellType::Virus
            )
        {
            score =
                static_cast<int>(
                    std::ceil(
                        rs.size * rs.size / 100.0f
                    )
                    );

            showText = score >= 99;
        }

        // Имя
        if (showText && !blob.name.empty())
        {
            float screenX;
            float screenY;

            camera.worldToScreen(
                rs.x,
                rs.y,
                static_cast<int>(screenW),
                static_cast<int>(screenH),
                screenX,
                screenY
            );

            float nameSize =
                std::max(
                    rs.size * 0.2f,
                    std::log2(
                        1.0f + rs.size / 50.0f
                    ) * 25.0f
                ) * 1.2f;

            float fontScale =
                (nameSize * viewportPixelScale / camera.zoom) / 70.0f;

            textRenderer.addText(
                font,
                blob.name,
                screenX,
                screenY,
                fontScale
            );
        }

        // Масса
        if (
            showText &&
            (
                blob.cellType == CellType::Player ||
                blob.cellType == CellType::Virus
                )
            )
        {
            float f10 =
                std::max(
                    rs.size * 0.2f,
                    std::log2(
                        1.0f + rs.size / 50.0f
                    ) * 25.0f
                ) * 1.2f;

            float massFontSize;
            float massWorldY;

            if (blob.cellType == CellType::Virus)
            {
                massFontSize =
                    f10 * 1.4f * virusFontMultiplier;

                massWorldY =
                    rs.y;
            }
            else
            {
                massFontSize =
                    f10 * 0.8f;

                massWorldY =
                    rs.y -
                    50.0f * rs.size / 100.0f;
            }

            std::string massText =
                std::to_string(score);

            float massScreenX;
            float massScreenY;

            camera.worldToScreen(
                rs.x,
                massWorldY,
                static_cast<int>(screenW),
                static_cast<int>(screenH),
                massScreenX,
                massScreenY
            );

            float massFontScale =
                (massFontSize * viewportPixelScale / camera.zoom) / 70.0f;

            textRenderer.addText(
                font,
                massText,
                massScreenX,
                massScreenY,
                massFontScale
            );
        }
    }

    textRenderer.end(
        font,
        screenW,
        screenH,
        1.0f,
        1.0f,
        1.0f
    );
}