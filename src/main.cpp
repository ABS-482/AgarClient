#define NOMINMAX

#include <glad/glad.h>
#include <SDL3/SDL.h>

#include "Core/FrameStats.h"
#include "Graphics/Shader.h"
#include "Graphics/Window.h"
#include "Graphics/CircleMesh.h"
#include "Graphics/Camera.h"
#include "Graphics/Shaders/CircleShader.h"
#include "Graphics/PlayerColors.h"
#include "Graphics/Font.h"
#include "Graphics/TextRenderer.h"
#include "Graphics/Shaders/TextShader.h"
#include "Graphics/SkinManager.h"
#include "Graphics/SkinMesh.h"
#include "Graphics/Shaders/SkinShader.h"
#include "Graphics/Shaders/CircleInstancedShader.h"
#include "Graphics/InstancedCircleRenderer.h"
#include "Input/InputManager.h"
#include "Input/InputState.h"
#include "Network/NetworkClient.h"
#include "Network/PacketHandler.h"
#include "Game/World.h"

#include <ixwebsocket/IXNetSystem.h>

#include <algorithm> // для std::clamp
#include <chrono>
#include <unordered_map>

#include <iomanip>
#include <iostream>
#include <sstream>
#include <tuple>

namespace
{
    // Временная функция-заглушка вместо реальной таблицы playercolors.
    // Даёт разным colorIndex визуально разные цвета, чтобы можно было
    // отличать сущности друг от друга уже сейчас.
    std::tuple<float, float, float> placeholderColor(const Blob& blob)
    {
        if (blob.cellType == CellType::Virus)
            return { 0.2f, 0.85f, 0.2f };

        if (blob.cellType == CellType::Food)
        {
            float hue = static_cast<float>(blob.colorIndex % 12) / 12.0f;
            return { 0.6f + 0.4f * hue, 0.6f, 0.9f - 0.3f * hue };
        }

        // Player / EjectedMass — хэшируем colorIndex в псевдослучайный цвет
        float seed = static_cast<float>(blob.colorIndex * 37 % 255) / 255.0f;
        return {
            0.3f + 0.6f * seed,
            0.3f + 0.6f * (1.0f - seed),
            0.5f + 0.5f * std::abs(0.5f - seed)
        };
    }
}

int main()
{
    ix::initNetSystem();

    Window window("AgarClient", 1280, 720);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    if (!window.isValid())
    {
        ix::uninitNetSystem();
        return 1;
    }

    Shader circleShader(CircleShader::vertex, CircleShader::fragment);
    Shader skinShader(SkinShader::vertex, SkinShader::fragment);
    SkinMesh skinMesh;

    Font font("C:/dev/AgarClient/assets/fonts/arial.otf", 70.0f, 1.0f);
    Shader textShader(TextShader::vertex, TextShader::fragment);
    TextRenderer textRenderer(textShader);

    GLint uCenter = circleShader.uniformLocation("uCenter");
    GLint uRadius = circleShader.uniformLocation("uRadius");
    GLint uCameraPos = circleShader.uniformLocation("uCameraPos");
    GLint uZoom = circleShader.uniformLocation("uZoom");
    GLint uScreenSize = circleShader.uniformLocation("uScreenSize");
    GLint uColor = circleShader.uniformLocation("uColor");

    GLint skinCenter = skinShader.uniformLocation("uCenter");
    GLint skinRadius = skinShader.uniformLocation("uRadius");
    GLint skinCameraPos = skinShader.uniformLocation("uCameraPos");
    GLint skinZoom = skinShader.uniformLocation("uZoom");
    GLint skinScreenSize = skinShader.uniformLocation("uScreenSize");
    GLint skinTexture = skinShader.uniformLocation("uSkin");

    CircleMesh circleMesh;
    Shader circleInstancedShader(CircleInstancedShader::vertex, CircleInstancedShader::fragment);
    GLint iCameraPos = circleInstancedShader.uniformLocation("uCameraPos");
    GLint iZoom = circleInstancedShader.uniformLocation("uZoom");
    GLint iScreenSize = circleInstancedShader.uniformLocation("uScreenSize");

    InstancedCircleRenderer foodRenderer;
    std::vector<float> foodInstanceData;
    Camera camera;

    World world;
    SkinManager skinManager;

    struct RenderState
    {
        float prevX = 0.0f;
        float prevY = 0.0f;
        float prevSize = 0.0f;

        float x = 0.0f;
        float y = 0.0f;
        float size = 0.0f;

        std::chrono::steady_clock::time_point lastSeenUpdate{};
        CellType lastCellType = CellType::Food;
        bool initialized = false;
    };

    std::unordered_map<uint32_t, RenderState> renderStates;

    struct DrawEntry
    {
        uint32_t id;
        const Blob* blob;
        RenderState* rs;
    };

    std::vector<DrawEntry> drawList;

    PacketHandler packetHandler(9, world);

    NetworkClient network(packetHandler);
    network.connect("wss://megasplit5k1.petridish.pw");
    network.setPlayerPassword("");

    InputManager inputManager;
    InputState input;
    FrameStats stats;

    bool running = true;

    bool mapCentered = false;

    while (running)
    {
        stats.beginFrame();

        running = inputManager.poll(input);

        auto blobs = world.snapshot();

        skinManager.processCompleted();

        if (!mapCentered)
        {
            World::MapBounds bounds = world.getMapBounds();

            if (bounds.valid)
            {
                float minX = static_cast<float>(bounds.minX);
                float minY = static_cast<float>(bounds.minY);
                float maxX = static_cast<float>(bounds.maxX);
                float maxY = static_cast<float>(bounds.maxY);

                float middleX = (minX + maxX) * 0.5f;
                float middleY = (minY + maxY) * 0.5f;

                camera.setBounds(minX, minY, maxX, maxY);
                camera.setManualTarget(middleX, middleY);

                camera.x = middleX;
                camera.y = middleY;

                camera.setZoomImmediate(0.0621f);

                mapCentered = true;
            }
        }

        if (input.mouseWheel != 0.0f)
        {
            camera.zoomBy(input.mouseWheel);
        }

        if (input.leftButtonJustPressed)
        {
            float worldX, worldY;
            camera.screenToWorld(
                input.mouseX, input.mouseY,
                static_cast<float>(window.width()),
                static_cast<float>(window.height()),
                worldX, worldY
            );

            camera.setManualTarget(worldX, worldY);
            network.sendSpectatePosition(worldX, worldY);
        }

        camera.update(static_cast<float>(stats.deltaTime()));

        glClearColor(0.06f, 0.06f, 0.09f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        constexpr float interpolationDuration = 0.12f;

        auto now = std::chrono::steady_clock::now();

        drawList.clear();
        drawList.reserve(blobs->size());

        foodInstanceData.clear();
        foodInstanceData.reserve(blobs->size() * 6);

        for (const auto& [id, blob] : *blobs)
        {
            RenderState& rs = renderStates[id];

            if (!rs.initialized)
            {
                rs.prevX = rs.x = blob.targetX;
                rs.prevY = rs.y = blob.targetY;
                rs.prevSize = rs.size = blob.targetSize;
                rs.lastSeenUpdate = blob.lastUpdateTime;
                rs.lastCellType = blob.cellType;
                rs.initialized = true;
            }
            else if (blob.cellType != rs.lastCellType)
            {
                rs.prevX = rs.x = blob.targetX;
                rs.prevY = rs.y = blob.targetY;
                rs.prevSize = rs.size = blob.targetSize;
                rs.lastSeenUpdate = blob.lastUpdateTime;
                rs.lastCellType = blob.cellType;
            }
            else if (blob.lastUpdateTime != rs.lastSeenUpdate)
            {
                rs.prevX = rs.x;
                rs.prevY = rs.y;
                rs.prevSize = rs.size;
                rs.lastSeenUpdate = blob.lastUpdateTime;
            }

            float elapsed = std::chrono::duration<float>(now - blob.lastUpdateTime).count();
            float t = std::clamp(elapsed / interpolationDuration, 0.0f, 1.0f);

            rs.x = rs.prevX + (blob.targetX - rs.prevX) * t;
            rs.y = rs.prevY + (blob.targetY - rs.prevY) * t;
            rs.size = rs.prevSize + (blob.targetSize - rs.prevSize) * t;

            if (blob.cellType == CellType::Food || blob.cellType == CellType::EjectedMass)
            {
                RGB rgb = getPlayerColor(blob.colorIndex);

                foodInstanceData.insert(foodInstanceData.end(), {
                    rs.x, rs.y, rs.size,
                    rgb.r / 255.0f, rgb.g / 255.0f, rgb.b / 255.0f
                    });
            }
            else
            {
                drawList.push_back({ id, &blob, &rs });
            }
        }

        // Обычная, ПРОСТАЯ и валидная сортировка — еды тут больше нет вообще.
        std::sort(
            drawList.begin(), drawList.end(),
            [](const DrawEntry& a, const DrawEntry& b)
            {
                return a.rs->size < b.rs->size;
            }
        );

        circleShader.use();
        circleShader.setVec2(uCameraPos, camera.x, camera.y);
        circleShader.setFloat(uZoom, camera.zoom);
        circleShader.setVec2(
            uScreenSize,
            static_cast<float>(window.width()),
            static_cast<float>(window.height())
        );

        skinShader.use();
        skinShader.setVec2(skinCameraPos, camera.x, camera.y);
        skinShader.setFloat(skinZoom, camera.zoom);
        skinShader.setVec2(
            skinScreenSize,
            static_cast<float>(window.width()),
            static_cast<float>(window.height())
        );

        constexpr float virusFontMultiplier = 1.6f;

        circleInstancedShader.use();
        circleInstancedShader.setVec2(iCameraPos, camera.x, camera.y);
        circleInstancedShader.setFloat(iZoom, camera.zoom);
        circleInstancedShader.setVec2(
            iScreenSize,
            static_cast<float>(window.width()),
            static_cast<float>(window.height())
        );

        foodRenderer.draw(foodInstanceData.data(), foodInstanceData.size() / 6);

        textRenderer.begin();

        for (const auto& entry : drawList)
        {
            const Blob& blob = *entry.blob;

            RenderState& rs = *entry.rs;

            // --- Круг ---
            circleShader.use();
            circleShader.setVec2(uCenter, rs.x, rs.y);
            circleShader.setFloat(uRadius, rs.size);

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
                RGB rgb = getPlayerColor(blob.colorIndex);

                circleShader.setVec3(
                    uColor,
                    rgb.r / 255.0f,
                    rgb.g / 255.0f,
                    rgb.b / 255.0f
                );
            }

            circleMesh.draw();

            // --- Скин ---
            bool wantsSkin =
                (blob.cellType == CellType::Player || blob.cellType == CellType::Virus) &&
                blob.skin != 0;

            if (wantsSkin)
            {
                GLuint texture = skinManager.getTexture(blob.skin);

                if (texture != 0)
                {
                    skinShader.use();
                    skinShader.setVec2(skinCenter, rs.x, rs.y);
                    skinShader.setFloat(skinRadius, rs.size);

                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, texture);
                    skinShader.setInt(skinTexture, 0);

                    skinMesh.draw();
                }
            }

            int score = 0;
            bool showText = true;

            if (blob.cellType == CellType::Player || blob.cellType == CellType::Virus)
            {
                score = static_cast<int>(std::ceil(rs.size * rs.size / 100.0f));
                showText = score >= 99;
            }

            // --- Имя ---
            if (showText && !blob.name.empty())
            {
                float screenX, screenY;
                camera.worldToScreen(
                    rs.x, rs.y,
                    window.width(), window.height(),
                    screenX, screenY
                );

                float nameSize = std::max(std::floor(0.3f * rs.size), 24.0f);

                if (blob.cellType == CellType::Virus)
                {
                    nameSize *= virusFontMultiplier;
                }

                float fontScale = (nameSize * camera.zoom) / 70.0f;

                textRenderer.addText(font, blob.name, screenX, screenY, fontScale);
            }

            // --- Масса ---
            if (showText && (blob.cellType == CellType::Player || blob.cellType == CellType::Virus))
            {
                float f10 = std::max(
                    rs.size * 0.2f,
                    std::log2(1.0f + rs.size / 50.0f) * 25.0f
                ) * 1.2f;

                float massFontSize;
                float massWorldY;

                if (blob.cellType == CellType::Virus)
                {
                    massFontSize = f10 * 1.4f * virusFontMultiplier;
                    massWorldY = rs.y;
                }
                else
                {
                    massFontSize = f10 * 0.8f;
                    massWorldY = rs.y + 50.0f * rs.size / 100.0f;
                }

                std::string massText = std::to_string(score);

                float massScreenX, massScreenY;
                camera.worldToScreen(
                    rs.x, massWorldY,
                    window.width(), window.height(),
                    massScreenX, massScreenY
                );

                float massFontScale = (massFontSize * camera.zoom) / 70.0f;

                textRenderer.addText(font, massText, massScreenX, massScreenY, massFontScale);
            }
        }

        textRenderer.end(
            font,
            static_cast<float>(window.width()),
            static_cast<float>(window.height()),
            1.0f, 1.0f, 1.0f
        );

        glBindTexture(GL_TEXTURE_2D, 0);

        for (auto it = renderStates.begin(); it != renderStates.end(); )
        {
            if (blobs->find(it->first) == blobs->end()) // было blobs.find/blobs.end
                it = renderStates.erase(it);
            else
                ++it;
        }

        window.swap();

        stats.endFrame(inputManager.mouseEventsThisFrame());

        if (stats.hasNewStats())
        {
            std::ostringstream title;
            title << "AgarClient | "
                << std::fixed << std::setprecision(0) << stats.fps()
                << " FPS | avg " << std::setprecision(3) << stats.averageFrameTimeMs()
                << " ms | entities " << blobs->size()
                << " | zoom " << std::setprecision(4) << camera.zoom;

            window.setTitle(title.str());
        }
    }

    ix::uninitNetSystem();

    return 0;
}