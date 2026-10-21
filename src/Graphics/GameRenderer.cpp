#include "GameRenderer.h"

#include "PlayerColors.h"
#include "../VirusGuard/VirusGuard.h"
#include "../FrameProfiler.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstddef>

namespace
{
    class AccumulatedTimer
    {
    public:
        explicit AccumulatedTimer(double& total) noexcept
            : m_total(total),
            m_start(Clock::now())
        {
        }

        ~AccumulatedTimer() noexcept
        {
            const auto elapsed = Clock::now() - m_start;

            m_total += std::chrono::duration<
                double,
                std::milli
            >(elapsed).count();
        }

        AccumulatedTimer(const AccumulatedTimer&) = delete;
        AccumulatedTimer& operator=(const AccumulatedTimer&) = delete;

    private:
        using Clock = std::chrono::steady_clock;

        double& m_total;
        Clock::time_point m_start;
    };
}

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
    camera(camera),
    lineRenderer(camera)
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

    uOutlineWidth =
        circleShader.uniformLocation("uOutlineWidth");

    uDepth = circleShader.uniformLocation("uDepth");

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

    skinBaseColor =
        skinShader.uniformLocation("uBaseColor");

    skinDepth = skinShader.uniformLocation("uDepth");

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
    for (const auto& [id, blob] : *blobs)
    {
        RenderState& rs = renderStates[id];

        if (!rs.initialized)
        {
            rs.x = blob.targetX;
            rs.y = blob.targetY;
            rs.size = blob.targetSize;

            rs.ox = blob.targetX;
            rs.oy = blob.targetY;
            rs.oSize = blob.targetSize;

            rs.nx = blob.targetX;
            rs.ny = blob.targetY;
            rs.nSize = blob.targetSize;

            rs.updateTime = blob.lastUpdateTime;

            rs.serverMoveX = blob.targetX;
            rs.serverMoveY = blob.targetY;
            rs.serverPosX = blob.targetX;
            rs.serverPosY = blob.targetY;

            rs.rawX = blob.targetX;
            rs.rawY = blob.targetY;
            rs.vx = 0.0f;
            rs.vy = 0.0f;

            rs.targetSize = blob.targetSize;
            rs.radiusDelta = 0.8f;
            rs.moveTimer = 0.0f;
            rs.updateInterval = 0.04f;
            rs.lerpDuration = 0.12f;

            rs.lastSeenUpdate = blob.lastUpdateTime;
            rs.lastCellType = blob.cellType;
            rs.initialized = true;

            continue;
        }

        if (blob.lastUpdateTime != rs.lastSeenUpdate)
        {
            rs.updatePos(blob.lastUpdateTime);

            rs.ox = rs.x;
            rs.oy = rs.y;
            rs.oSize = rs.size;

            rs.nx = blob.targetX;
            rs.ny = blob.targetY;
            rs.nSize = blob.targetSize;

            rs.updateTime = blob.lastUpdateTime;

            rs.targetSize = blob.targetSize;
            rs.lastSeenUpdate = blob.lastUpdateTime;
            rs.lastCellType = blob.cellType;

            rs.serverMoveX = rs.ox;
            rs.serverMoveY = rs.oy;
            rs.serverPosX = rs.nx;
            rs.serverPosY = rs.ny;

            rs.rawX = rs.nx;
            rs.rawY = rs.ny;
            rs.vx = 0.0f;
            rs.vy = 0.0f;
            rs.moveTimer = 0.0f;
            rs.lerpDuration = 0.12f;
        }
    }

    for (auto it = renderStates.begin(); it != renderStates.end(); )
    {
        if (blobs->find(it->first) == blobs->end())
            it = renderStates.erase(it);
        else
            ++it;
    }
}

static int getBlobMass(const Blob& blob)
{
    return static_cast<int>(
        blob.targetSize *
        blob.targetSize /
        100.0f
        );
}

void GameRenderer::updatePositions(
    std::unordered_map<uint32_t, RenderState>& renderStates)
{
    const auto now = std::chrono::steady_clock::now();

    for (auto& [id, rs] : renderStates)
    {
        if (rs.initialized)
            rs.updatePos(now);
    }
}

void GameRenderer::draw(
    const WorldSnapshot& blobs,
    const std::vector<uint32_t>& ownedIds,
    std::unordered_map<uint32_t, RenderState>& renderStates,
    float screenW,
    float screenH,
    float targetWorldX,
    float targetWorldY,
    FrameProfiler& profiler,
    const VirusGuard* virusGuard,
    bool splitPreviewHeld
)
{
    glClearColor(
        242.0f / 255.0f,
        251.0f / 255.0f,
        255.0f / 255.0f,
        1.0f
    );

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_TRUE);
    glClearDepth(1.0);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    SplitPredictionResult splitResult;

    if (
        splitPreviewHeld &&
        !ownedIds.empty()
        )
    {
        FrameProfiler::ScopedTimer timer(
            profiler,
            FrameProfiler::GameRenderSplitSimulate
        );

        std::vector<uint32_t> sortedOwnedIds =
            ownedIds;

        std::sort(
            sortedOwnedIds.begin(),
            sortedOwnedIds.end()
        );

        splitResult =
            splitPrediction.simulate(
                blobs,
                sortedOwnedIds,
                renderStates,
                targetWorldX,
                targetWorldY
            );
    }

    int myMass = 0;

    {
        FrameProfiler::ScopedTimer timer(
            profiler,
            FrameProfiler::GameRenderPrepare
        );

        /*
         * Определяем самую большую собственную часть.
         *
         * largestId = ID самой большой части
         * myMass    = масса самой большой части
         *
         * Все собственные части с ID < largestId
         * считаются leading.
         */

        uint32_t largestId = 0;
        float largestSize = -1.0f;
        bool largestFound = false;
        for (auto& [id, rs] : renderStates)
        {
            rs.isLeading = false;
        }

        // Ищем самую большую собственную часть.
        for (const uint32_t id : ownedIds)
        {
            auto blobIt =
                blobs->find(id);

            if (blobIt == blobs->end())
                continue;

            const Blob& blob =
                blobIt->second;

            if (
                !largestFound ||
                blob.targetSize > largestSize
                )
            {
                largestSize = blob.targetSize;
                largestId = id;
                myMass = getBlobMass(blob);
                largestFound = true;
            }
        }

        // Все собственные части с ID меньше ID крупнейшей
        // считаются ведущими.
        if (largestFound)
        {
            for (const uint32_t id : ownedIds)
            {
                if (id >= largestId)
                    continue;

                auto rsIt =
                    renderStates.find(id);

                if (rsIt == renderStates.end())
                    continue;

                rsIt->second.isLeading = true;
            }
        }

        // Подготавливаем списки отрисовки.
        drawList.clear();
        drawList.reserve(blobs->size());

        foodInstanceData.clear();
        foodInstanceData.reserve(
            blobs->size() * 7
        );

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
                        rgb.b / 255.0f,
                        0.95f
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
    }
    {
        FrameProfiler::ScopedTimer timer(
            profiler,
            FrameProfiler::GameRenderSort
        );

        std::sort(
            drawList.begin(),
            drawList.end(),
            [](const DrawEntry& a, const DrawEntry& b)
            {
                if (a.rs->size != b.rs->size)
                    return a.rs->size < b.rs->size;

                return a.id < b.id;
            }
        );
}

    // Глубина должна соответствовать порядку отрисовки клеток:
// маленькие дальше, большие ближе.
    const float depthStep =
        1.6f / (static_cast<float>(drawList.size()) + 1.0f);

    // Для каждой клетки запоминаем состояние её текстуры.
// Готовый скин исключает базовый круг из instanced-пакета.
    {
        FrameProfiler::ScopedTimer timer(
            profiler,
            FrameProfiler::GameRenderSkinLookup
        );

        skinTexturesByDrawIndex.resize(drawList.size());

        for (std::size_t drawIndex = 0;
            drawIndex < drawList.size();
            ++drawIndex)
        {
            const DrawEntry& entry = drawList[drawIndex];
            const Blob& blob = *entry.blob;
            const RenderState& rs = *entry.rs;

            GLuint texture = 0;

            if (
                (blob.cellType == CellType::Player ||
                    blob.cellType == CellType::Virus) &&
                blob.skin != 0
                )
            {
                // Не блокирует кадр. Если текстура ещё загружается,
                // вернётся 0, и базовый круг останется в общем пакете.
                texture = skinManager.getTexture(blob.skin);
            }

            skinTexturesByDrawIndex[drawIndex] = texture;

            // Готовый скин будет отрисован отдельно ниже.
            // Не добавляем под него базовый цветной круг.
            if (texture != 0)
                continue;

            const float cellDepth =
                0.8f -
                depthStep * static_cast<float>(drawIndex + 1);

            float r;
            float g;
            float b;

            if (blob.cellType == CellType::Virus)
            {
                r = 1.0f;
                g = 153.0f / 255.0f;
                b = 0.0f;
            }
            else
            {
                const RGB rgb = getPlayerColor(blob.colorIndex);

                r = rgb.r / 255.0f;
                g = rgb.g / 255.0f;
                b = rgb.b / 255.0f;
            }

            foodInstanceData.insert(
                foodInstanceData.end(),
                {
                    rs.x,
                    rs.y,
                    rs.size,
                    r,
                    g,
                    b,
                    cellDepth
                }
            );
        }
    }

    {
        FrameProfiler::ScopedTimer timer(
            profiler,
            FrameProfiler::GameRenderShaderSetup
        );

        // Настройка circle shader
        circleShader.use();

        circleShader.setVec2(
            uCameraPos,
            camera.offsetX,
            camera.offsetY
        );

        circleShader.setFloat(
            uZoom,
            camera.ratio
        );

        circleShader.setVec2(
            uScreenSize,
            screenW,
            screenH
        );

        circleShader.setVec2(
            uViewportSize,
            screenW,
            screenH
        );

        // Настройка skin shader
        skinShader.use();

        skinShader.setVec2(
            skinCameraPos,
            camera.offsetX,
            camera.offsetY
        );

        skinShader.setFloat(
            skinZoom,
            camera.ratio
        );

        skinShader.setVec2(
            skinScreenSize,
            screenW,
            screenH
        );

        skinShader.setVec2(
            skinViewportSize,
            screenW,
            screenH
        );

        // Настройка instanced food shader
        circleInstancedShader.use();

        circleInstancedShader.setVec2(
            iCameraPos,
            camera.offsetX,
            camera.offsetY
        );

        circleInstancedShader.setFloat(
            iZoom,
            camera.ratio
        );

        circleInstancedShader.setVec2(
            iScreenSize,
            screenW,
            screenH
        );

        circleInstancedShader.setVec2(
            iViewportSize,
            screenW,
            screenH
        );
    }

    {
        FrameProfiler::ScopedTimer timer(
            profiler,
            FrameProfiler::GameRenderInstanced
        );

        foodRenderer.draw(
            foodInstanceData.data(),
            foodInstanceData.size() / 7
        );
    }

    constexpr float virusFontMultiplier = 1.6f;

    textRenderer.begin();

    double entityOutlinesMs = 0.0;
    double skinDrawMs = 0.0;
    double textBuildMs = 0.0;

    {
        FrameProfiler::ScopedTimer timer(
            profiler,
            FrameProfiler::GameRenderEntities
        );

        for (std::size_t drawIndex = 0;
            drawIndex < drawList.size();
            ++drawIndex)
        {
            const auto& entry = drawList[drawIndex];

            const float cellDepth =
                0.8f -
                depthStep * static_cast<float>(drawIndex + 1);

            const float outlineDepth =
                cellDepth + depthStep * 0.25f;

            const float skinDepthValue =
                cellDepth - depthStep * 0.1f;

            const float textDepth =
                cellDepth - depthStep * 0.2f;

            const Blob& blob =
                *entry.blob;

            RenderState& rs =
                *entry.rs;

            /*
             * Оранжевая обводка ведущей части.
             *
             * Сначала рисуем чуть больший оранжевый круг,
             * потом поверх него обычную клетку.
             */
            if (
                rs.isLeading &&
                blob.cellType == CellType::Player
                )
            {
                AccumulatedTimer sectionTimer(entityOutlinesMs);
                constexpr float outlineOffset = 20.0f;
                constexpr float outlineThickness = 10.0f;

                const float outerRadius =
                    rs.size + outlineOffset;

                circleShader.use();

                circleShader.setVec2(
                    uCenter,
                    rs.x,
                    rs.y
                );

                circleShader.setFloat(
                    uRadius,
                    outerRadius
                );

                const float outlineWidthNormalized =
                    outlineThickness / outerRadius;

                circleShader.setFloat(
                    uOutlineWidth,
                    outlineWidthNormalized
                );

                circleShader.setVec3(
                    uColor,
                    1.0f,
                    0.0f,
                    0.0f
                );

                circleShader.setFloat(uDepth, outlineDepth);

                circleMesh.draw();
            }

            // Обводка калькулятора массы
            if (
                !splitPreviewHeld &&
                blob.cellType == CellType::Player &&
                blob.targetSize > 24.0f &&
                !ownedIds.empty()
                )
            {
                AccumulatedTimer sectionTimer(entityOutlinesMs);
                bool isOwned = false;

                for (const uint32_t ownedId : ownedIds)
                {
                    if (ownedId == entry.id)
                    {
                        isOwned = true;
                        break;
                    }
                }

                // Калькулятор нужен только для чужих клеток.
                if (!isOwned)
                {
                    const int thisMass =
                        getBlobMass(blob);

                    float outlineR = 1.0f;
                    float outlineG = 1.0f;
                    float outlineB = 0.0f;

                    if (thisMass > 2.6f * myMass)
                    {
                        // #FF3C3C
                        outlineR = 1.0f;
                        outlineG = 60.0f / 255.0f;
                        outlineB = 60.0f / 255.0f;
                    }
                    else if (thisMass > 1.3f * myMass)
                    {
                        // #FFBF3D
                        outlineR = 1.0f;
                        outlineG = 191.0f / 255.0f;
                        outlineB = 61.0f / 255.0f;
                    }
                    else if (2.6f * thisMass < myMass)
                    {
                        // #44F720
                        outlineR = 68.0f / 255.0f;
                        outlineG = 247.0f / 255.0f;
                        outlineB = 32.0f / 255.0f;
                    }
                    else if (1.3f * thisMass < myMass)
                    {
                        // #00AA00
                        outlineR = 0.0f;
                        outlineG = 170.0f / 255.0f;
                        outlineB = 0.0f;
                    }

                    constexpr float outlineOffset = 20.0f;
                    constexpr float outlineThickness = 10.0f;

                    const float outerRadius =
                        rs.size + outlineOffset;

                    circleShader.use();

                    circleShader.setVec2(
                        uCenter,
                        rs.x,
                        rs.y
                    );

                    circleShader.setFloat(
                        uRadius,
                        outerRadius
                    );

                    const float outlineWidthNormalized =
                        outlineThickness / outerRadius;

                    circleShader.setFloat(
                        uOutlineWidth,
                        outlineWidthNormalized
                    );

                    circleShader.setVec3(
                        uColor,
                        outlineR,
                        outlineG,
                        outlineB
                    );

                    circleShader.setFloat(uDepth, outlineDepth);
                    circleMesh.draw();
                }
            }

            // Скин
            const bool wantsSkin =
                (
                    blob.cellType == CellType::Player ||
                    blob.cellType == CellType::Virus
                    ) &&
                blob.skin != 0;

            if (wantsSkin)
            {
                AccumulatedTimer sectionTimer(skinDrawMs);

                const GLuint texture =
                    skinTexturesByDrawIndex[drawIndex];

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

                    if (blob.cellType == CellType::Virus)
                    {
                        skinShader.setVec3(
                            skinBaseColor,
                            1.0f,
                            153.0f / 255.0f,
                            0.0f
                        );
                    }
                    else
                    {
                        const RGB rgb = getPlayerColor(blob.colorIndex);

                        skinShader.setVec3(
                            skinBaseColor,
                            rgb.r / 255.0f,
                            rgb.g / 255.0f,
                            rgb.b / 255.0f
                        );
                    }

                    skinShader.setFloat(
                        skinDepth,
                        skinDepthValue
                    );

                    skinMesh.draw();
                }
            }

            {
                AccumulatedTimer sectionTimer(textBuildMs);

                const bool isPlayerOrVirus =
                    blob.cellType == CellType::Player ||
                    blob.cellType == CellType::Virus;

                int score = 0;
                bool showText = true;

                if (isPlayerOrVirus)
                {
                    score =
                        static_cast<int>(
                            std::ceil(
                                rs.size * rs.size / 100.0f
                            )
                            );

                    showText = score >= 99;
                }

                const bool hasName = !blob.name.empty();

                float textBaseSize = 0.0f;

                if (showText && (hasName || isPlayerOrVirus))
                {
                    textBaseSize =
                        std::max(
                            rs.size * 0.2f,
                            std::log2(
                                1.0f + rs.size / 50.0f
                            ) * 25.0f
                        ) * 1.2f;
                }

                // Имя
                if (showText && hasName)
                {
                    float screenX;
                    float screenY;

                    camera.worldToScreen(
                        rs.x,
                        rs.y,
                        screenX,
                        screenY
                    );

                    const float nameSize = textBaseSize;

                    const float fontScale = (nameSize * camera.ratio) / 70.0f;

                    textRenderer.addText(
                        font,
                        blob.name,
                        screenX,
                        screenY,
                        fontScale,
                        textDepth
                    );
                }

                // Масса
                if (showText && isPlayerOrVirus)
                {
                    const float f10 = textBaseSize;

                    float massFontSize;
                    float massWorldY;

                    if (blob.cellType == CellType::Virus)
                    {
                        massFontSize =
                            f10 * 1.4f * virusFontMultiplier;

                        massWorldY = rs.y;
                    }
                    else
                    {
                        massFontSize = f10 * 0.8f;

                        massWorldY =
                            rs.y +
                            50.0f * rs.size / 100.0f;
                    }

                    const std::string massText = std::to_string(score);

                    float massScreenX;
                    float massScreenY;

                    camera.worldToScreen(
                        rs.x,
                        massWorldY,
                        massScreenX,
                        massScreenY
                    );

                    const float massFontScale =
                        (massFontSize * camera.ratio) / 70.0f;

                    textRenderer.addText(
                        font,
                        massText,
                        massScreenX,
                        massScreenY,
                        massFontScale,
                        textDepth,
                        false
                    );
                }
            }
        }
    }
    profiler.record(
        FrameProfiler::GameRenderEntityOutlines,
        entityOutlinesMs
    );

    profiler.record(
        FrameProfiler::GameRenderSkinDraw,
        skinDrawMs
    );

    profiler.record(
        FrameProfiler::GameRenderTextBuild,
        textBuildMs
    );
    // ------------------------------------------------------------
    // Split Preview
    // ------------------------------------------------------------

    if (!splitResult.cells.empty())
    {
        FrameProfiler::ScopedTimer timer(
            profiler,
            FrameProfiler::GameRenderSplitPreview
        );

        circleShader.use();
        circleShader.setFloat(uDepth, -0.95f);

        constexpr float outlineOffset = 20.0f;
        constexpr float outlineThickness = 10.0f;

        for (const SplitPredictionCell& predicted : splitResult.cells)
        {
            float r = 1.0f;
            float g = 1.0f;
            float b = 1.0f;

            // Попадание на вирус.
            if (predicted.hitsVirus)
            {
                r = 1.0f;
                g = 0.0f;
                b = 0.0f;
            }
            // Чужая клетка может съесть predicted.
            else if (predicted.dangerLevel == 3)
            {
                r = 1.0f;
                g = 0.25f;
                b = 0.25f;
            }
            // Predicted может съесть чужую клетку.
            else if (predicted.dangerLevel == 2)
            {
                r = 0.2f;
                g = 1.0f;
                b = 0.2f;
            }
            // Простое пересечение.
            else if (predicted.dangerLevel == 1)
            {
                r = 1.0f;
                g = 0.8f;
                b = 0.2f;
            }

            const float outerRadius =
                predicted.radius + outlineOffset;

            circleShader.setVec2(
                uCenter,
                predicted.x,
                predicted.y
            );

            circleShader.setFloat(
                uRadius,
                outerRadius
            );

            const float outlineWidthNormalized =
                outlineThickness / outerRadius;

            circleShader.setFloat(
                uOutlineWidth,
                outlineWidthNormalized
            );

            circleShader.setVec3(
                uColor,
                r,
                g,
                b
            );

            circleMesh.draw();
        }

        // ------------------------------------------------------------
        // Обводка чужих клеток, которые predicted-клетка
        // может съесть.
        // ------------------------------------------------------------

        for (const SplitPredictionEnemy& enemy :
            splitResult.enemies)
        {
            if (enemy.dangerLevel != 2)
                continue;

            const auto rsIt =
                renderStates.find(enemy.id);

            if (rsIt == renderStates.end())
                continue;

            auto blobIt =
                blobs->find(enemy.id);

            if (blobIt == blobs->end())
                continue;

            const Blob& blob =
                blobIt->second;

            if (blob.cellType != CellType::Player)
                continue;

            const RenderState& rs =
                rsIt->second;

            if (rs.size <= 0.0f)
                continue;

            constexpr float outlineOffset = 20.0f;
            constexpr float outlineThickness = 10.0f;

            const float outerRadius =
                rs.size + outlineOffset;

            circleShader.setVec2(
                uCenter,
                rs.x,
                rs.y
            );

            circleShader.setFloat(
                uRadius,
                outerRadius
            );

            const float outlineWidthNormalized =
                outlineThickness / outerRadius;

            circleShader.setFloat(
                uOutlineWidth,
                outlineWidthNormalized
            );

            // Зелёная обводка:
            // predicted-клетка может съесть этого enemy.
            circleShader.setVec3(
                uColor,
                0.2f,
                1.0f,
                0.2f
            );

            circleMesh.draw();
        }

        // Сбрасываем режим обводки перед дальнейшей отрисовкой.
        circleShader.setFloat(
            uOutlineWidth,
            0.0f
        );
    }

    glDepthMask(GL_FALSE);

    {
        FrameProfiler::ScopedTimer timer(
            profiler,
            FrameProfiler::GameRenderTextFlush
        );

        textRenderer.end(
            font,
            screenW,
            screenH,
            1.0f,
            1.0f,
            1.0f
        );
    }

    glDepthMask(GL_TRUE);
    glDisable(GL_DEPTH_TEST);
}