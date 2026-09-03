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

    Font font("C:/Windows/Fonts/arial.ttf", 70.0f);
    Shader textShader(TextShader::vertex, TextShader::fragment);
    TextRenderer textRenderer;

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
        bool initialized = false;
    };

    std::unordered_map<uint32_t, RenderState> renderStates;

    PacketHandler packetHandler(9, world);

    NetworkClient network(packetHandler);
    network.connect("wss://megasplit5k5.petridish.pw");
    network.setPlayerPassword("");

    InputManager inputManager;
    InputState input;
    FrameStats stats;

    bool running = true;

    while (running)
    {
        stats.beginFrame();

        running = inputManager.poll(input);

        auto blobs = world.snapshot();

        camera.fitToBlobs(blobs);

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

        circleShader.use();
        circleShader.setVec2(uCameraPos, camera.x, camera.y);
        circleShader.setFloat(uZoom, camera.zoom);
        circleShader.setVec2(
            uScreenSize,
            static_cast<float>(window.width()),
            static_cast<float>(window.height())
        );

        constexpr float interpolationDuration = 0.12f;

        auto now = std::chrono::steady_clock::now();

        for (const auto& [id, blob] : blobs)
        {
            RenderState& rs = renderStates[id];

            if (!rs.initialized)
            {
                rs.prevX = rs.x = blob.targetX;
                rs.prevY = rs.y = blob.targetY;
                rs.prevSize = rs.size = blob.targetSize;
                rs.lastSeenUpdate = blob.lastUpdateTime;
                rs.initialized = true;
            }
            else if (blob.lastUpdateTime != rs.lastSeenUpdate)
            {
                // Пришёл новый world update — фиксируем текущую
                // отрисованную позицию как новую точку "откуда".
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

            circleShader.setVec2(uCenter, rs.x, rs.y);
            circleShader.setFloat(uRadius, rs.size);

            RGB rgb = getPlayerColor(blob.colorIndex);
            circleShader.setVec3(uColor, rgb.r / 255.0f, rgb.g / 255.0f, rgb.b / 255.0f);

            circleMesh.draw();
            if (blob.skin > 0)
            {
                GLuint texture = skinManager.getTexture(blob.skin);

                if (texture != 0)
                {
                    skinShader.use();

                    skinShader.setVec2(skinCenter, rs.x, rs.y);
                    skinShader.setFloat(skinRadius, rs.size);

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
                        static_cast<float>(window.width()),
                        static_cast<float>(window.height())
                    );

                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, texture);

                    skinShader.setInt(skinTexture, 0);

                    skinMesh.draw();

                    glBindTexture(GL_TEXTURE_2D, 0);
                }
            }
        }

        for (const auto& [id, blob] : blobs)
        {
            RenderState& rs = renderStates[id];

            // Имя
            if (!blob.name.empty())
            {
                float screenX, screenY;
                camera.worldToScreen(
                    rs.x, rs.y,
                    window.width(), window.height(),
                    screenX, screenY
                );

                float nameSize = std::max(std::floor(0.3f * rs.size), 24.0f);
                float fontScale = (nameSize * camera.zoom) / 70.0f;

                textRenderer.drawCentered(
                    font,
                    textShader,
                    blob.name,
                    screenX,
                    screenY,
                    static_cast<float>(window.width()),
                    static_cast<float>(window.height()),
                    1.0f, 1.0f, 1.0f,
                    fontScale
                );
            }

            // Масса
            if (blob.cellType == CellType::Player ||
                blob.cellType == CellType::Virus)
            {
                float f10 = std::max(
                    rs.size * 0.2f,
                    std::log2(1.0f + rs.size / 50.0f) * 25.0f
                ) * 1.2f;

                float massFontSize;
                float massWorldY;

                if (blob.cellType == CellType::Virus)
                {
                    massFontSize = f10 * 1.4f;
                    massWorldY = rs.y;
                }
                else
                {
                    massFontSize = f10 * 0.8f;
                    massWorldY = rs.y + 50.0f * rs.size / 100.0f;
                }

                int score = static_cast<int>(
                    std::ceil(rs.size * rs.size / 100.0f)
                    );

                std::string massText = std::to_string(score);

                float massScreenX, massScreenY;
                camera.worldToScreen(
                    rs.x, massWorldY,
                    window.width(), window.height(),
                    massScreenX, massScreenY
                );

                float massFontScale =
                    (massFontSize * camera.zoom) / 70.0f;

                textRenderer.drawCentered(
                    font,
                    textShader,
                    massText,
                    massScreenX,
                    massScreenY,
                    static_cast<float>(window.width()),
                    static_cast<float>(window.height()),
                    1.0f, 1.0f, 1.0f,
                    massFontScale
                );
            }
        }

        for (auto it = renderStates.begin(); it != renderStates.end(); )
        {
            if (blobs.find(it->first) == blobs.end())
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
                << " ms | entities " << blobs.size();

            window.setTitle(title.str());
        }
    }

    ix::uninitNetSystem();

    return 0;
}