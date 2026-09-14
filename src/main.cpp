#define NOMINMAX

#include <glad/glad.h>
#include <SDL3/SDL.h>
#include <cctype>

#include "Core/FrameStats.h"
#include "Core/FrameLimiter.h"
#include "Core/GameModes.h"
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
#include "Graphics/UIPanel.h"
#include "Input/InputManager.h"
#include "Input/InputState.h"
#include "Network/NetworkClient.h"
#include "Network/PacketHandler.h"
#include "Network/ServerListFetcher.h"
#include "Game/World.h"
#include "UI/ModeIconManager.h"
#include "Graphics/IconRenderer.h"

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

enum class AppState
{
    SelectingMode,
    SelectingServer,
    Playing
};

int main()
{
    ix::initNetSystem();

    Window window("AgarClient", 1280, 720);

    if (!window.isValid())
    {
        ix::uninitNetSystem();
        return 1;
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    Shader circleShader(CircleShader::vertex, CircleShader::fragment);
    Shader skinShader(SkinShader::vertex, SkinShader::fragment);
    SkinMesh skinMesh;

    Font font("C:/dev/AgarClient/assets/fonts/arial.otf", 70.0f, 1.0f);
    Font menuFont("C:/dev/AgarClient/assets/fonts/arial.otf", 85.0f, 0);
    Font gmFont("C:/dev/AgarClient/assets/fonts/arial.otf", 100.0f, 0);

    Shader textShader(TextShader::vertex, TextShader::fragment);
    UIPanel uiPanel;
    IconRenderer iconRenderer;

    std::vector<GLuint> gameModeIconTextures(gameModes.size(), 0);

    for (size_t i = 0; i < gameModes.size(); ++i)
    {
        gameModeIconTextures[i] = iconRenderer.loadTexture(
            "C:/dev/AgarClient/" + gameModes[i].iconPath
        );
    }
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
    FrameLimiter frameLimiter(window.refreshRate());
    World world;
    SkinManager skinManager;
    ModeIconManager modeIconManager;

    modeIconManager.load(
        "MEGASPLIT",
        "assets/icons/ms.png"
    );

    modeIconManager.load(
        "MEGASPLIT5K",
        "assets/icons/ms.png"
    );

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
    packetHandler.setNetworkClient(network);
    network.setPlayerPassword("");
    network.setNickname("Android Player");
    network.setPlayerColor(6); // любой индекс из вашей таблицы PlayerColors, подберите на вкус
    

    InputManager inputManager;
    InputState input;
    FrameStats stats;

    bool running = true;

    bool mapCentered = false;

    float aimXOld = 0.0f;
    float aimYOld = 0.0f;
    float bestScoreSoFar = 0.0f;
    float currentScore = 0.0f; // <- добавить рядом
    bool hasAimOld = false;
    std::chrono::steady_clock::time_point lastAimSendTime{};
    std::chrono::steady_clock::time_point lastMacroShotTime{};

    auto serverList = ServerListFetcher::fetch();

    AppState appState = AppState::SelectingServer;
    int selectedServerIndex = 0;
    int menuScrollOffset = 0;
    int selectedModeIndex = 0;

    std::string selectedMode = gameModes[selectedModeIndex].id;

    std::cout << "Fetched " << serverList.size() << " servers:\n";

    for (const auto& s : serverList)
    {
        std::cout << "  [" << s.id << "] " << s.sname
            << " mode=" << s.mode
            << " online=" << s.online << "/" << s.connectlimit
            << " addr=" << s.address << '\n';
    }

    while (running)
    {
        stats.beginFrame();

        frameLimiter.beginFrame();

        if (input.cycleFpsLimitPressed)
        {
            frameLimiter.cycleMode();
        }

        running = inputManager.poll(input);

        if (appState == AppState::Playing)
        {

            if (input.spawnRequestPressed)
            {
                network.requestSpawn();
            }

            auto blobs = world.snapshot();

            auto ownedIds = world.getOwnedIds();

            auto trySendFreshAim = [&](bool bypassRateLimit) -> bool
                {
                    float aimX, aimY;
                    camera.screenToWorld(
                        input.mouseX, input.mouseY,
                        static_cast<float>(window.width()),
                        static_cast<float>(window.height()),
                        aimX, aimY
                    );

                    bool changedEnough = !hasAimOld ||
                        std::abs(aimXOld - aimX) >= 0.01f ||
                        std::abs(aimYOld - aimY) >= 0.01f;

                    if (!changedEnough)
                        return false;

                    auto now = std::chrono::steady_clock::now();

                    if (!bypassRateLimit && (now - lastAimSendTime < std::chrono::milliseconds(4)))
                        return false;

                    lastAimSendTime = now;
                    aimXOld = aimX;
                    aimYOld = aimY;
                    hasAimOld = true;

                    network.sendAimPosition(std::floor(aimX), std::floor(aimY));
                    return true;
                };

            if ((input.splitRequested || input.ejectMassRequested) && !ownedIds.empty())
            {
                trySendFreshAim(true);

                if (input.splitRequested)
                {
                    network.requestSplit();
                }

                if (input.ejectMassRequested)
                {
                    network.requestEjectMass();
                }
            }

            if (input.ejectMassKeyHeld && !ownedIds.empty())
            {
                auto nowMacro = std::chrono::steady_clock::now();

                if (nowMacro - lastMacroShotTime >= std::chrono::milliseconds(40))
                {
                    lastMacroShotTime = nowMacro;

                    trySendFreshAim(true);
                    network.requestEjectMass();
                }
            }

            if (!ownedIds.empty())
            {
                float sumX = 0.0f;
                float sumY = 0.0f;
                int count = 0;

                float totalSize = 0.0f;

                currentScore = 0.0f;

                for (uint32_t id : ownedIds)
                {
                    auto it = blobs->find(id);

                    if (it != blobs->end())
                    {
                        auto rsIt = renderStates.find(id);

                        float sx, sy, ssize;

                        if (rsIt != renderStates.end())
                        {
                            sx = rsIt->second.x;
                            sy = rsIt->second.y;
                            ssize = rsIt->second.size;
                        }
                        else
                        {
                            sx = it->second.targetX;
                            sy = it->second.targetY;
                            ssize = it->second.targetSize;
                        }

                        sumX += sx;
                        sumY += sy;
                        totalSize += ssize;
                        currentScore += (ssize * ssize) / 100.0f;

                        ++count;
                    }
                }

                bestScoreSoFar = std::max(bestScoreSoFar, currentScore);

                if (totalSize > 0.0f)
                {
                    float sizeFactor = std::pow(std::min(64.0f / totalSize, 1.0f), 0.4f);
                    camera.setSizeZoomFactor(sizeFactor);
                }

                if (count > 0)
                {
                    float avgX = sumX / count;
                    float avgY = sumY / count;

                    camera.setManualTarget(avgX, avgY);
                }
            }

            if (!ownedIds.empty())
            {
                trySendFreshAim(false);
            }

            skinManager.processCompleted();

            network.update();
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
                network.sendAimPosition(worldX, worldY);
            }

            if (!ownedIds.empty())
            {
                camera.setZoomLimits(0.2f, 6.0f, 0.05f, 2.0f);

                camera.snapTowardsTarget(static_cast<float>(stats.deltaTime()), 0.01667f);
                camera.updateZoomOnly(static_cast<float>(stats.deltaTime()));
            }
            else
            {
                camera.setZoomLimits(0.2f, 1.5f, 0.05f, 0.4f);

                camera.update(static_cast<float>(stats.deltaTime()));
            }

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

            constexpr float rectR = 0.1647f;
            constexpr float rectG = 0.3922f;
            constexpr float rectB = 0.5882f;
            constexpr float rectA = 0.5f;

            textRenderer.begin(); // ОДИН общий batch на весь UI-текст этого кадра

            auto leaderboard = world.getLeaderboard();

            if (!leaderboard.empty())
            {
                constexpr int maxVisible = 10;
                constexpr float rowHeight = 28.0f;
                constexpr float padding = 16.0f;

                int visibleCount = std::min(static_cast<int>(leaderboard.size()), maxVisible);

                float panelWidth = 240.0f;
                float panelHeight = padding * 2.0f + rowHeight * visibleCount;

                float screenW = static_cast<float>(window.width());
                float screenH = static_cast<float>(window.height());

                float panelCenterX = screenW - panelWidth * 0.5f - 16.0f;
                float panelCenterY = panelHeight * 0.5f + 16.0f;

                uiPanel.draw(
                    panelCenterX, panelCenterY,
                    panelWidth, panelHeight,
                    0.0f, //cornerRadius = 0 → прямоугольник
                    rectR, rectG, rectB, rectA,
                    0.0f, 0.0f, 0.0f, 0.0f,
                    0.0f,
                    screenW, screenH
                );

                float topY = panelCenterY - panelHeight * 0.5f + padding;
                float rowX = screenW - panelWidth - 16.0f + padding;

                for (int i = 0; i < visibleCount; ++i)
                {
                    const auto& entry = leaderboard[i];
                    std::string line = std::to_string(i + 1) + ". " + entry.name;

                    float rowY = topY + rowHeight * i + rowHeight * 0.5f;

                    textRenderer.addTextLeftAligned(font, line, rowX, rowY, 0.28f);
                }
            }

            auto chatMessages = world.getChatMessages();

            if (!chatMessages.empty())
            {
                constexpr int maxVisible = 14;
                constexpr float rowHeight = 22.0f;
                constexpr float padding = 12.0f;

                int visibleCount = std::min(static_cast<int>(chatMessages.size()), maxVisible);

                float panelWidth = 300.0f;
                float panelHeight = padding * 2.0f + rowHeight * maxVisible; // <- фиксированная высота, всегда под maxVisible строк

                float screenW = static_cast<float>(window.width());
                float screenH = static_cast<float>(window.height());

                float panelCenterX = panelWidth * 0.5f + 10.0f;
                float hudReservedHeight = ownedIds.empty() ? 16.0f : 52.0f;
                float panelCenterY = screenH - hudReservedHeight - panelHeight * 0.5f;

                uiPanel.draw(
                    panelCenterX, panelCenterY,
                    panelWidth, panelHeight,
                    12.0f,
                    0.08f, 0.08f, 0.12f, 0.65f,
                    1.0f, 1.0f, 1.0f, 0.12f,
                    1.5f,
                    screenW, screenH
                );

                float topY = panelCenterY + panelHeight * 0.5f - padding - rowHeight * visibleCount;
                int startIdx = static_cast<int>(chatMessages.size()) - visibleCount;

                for (int i = 0; i < visibleCount; ++i)
                {
                    const auto& msg = chatMessages[startIdx + i];

                    std::string line = msg.isPlayerEnter
                        ? msg.name + " enters the game"
                        : msg.name + ": " + msg.message;

                    float rowY = topY + rowHeight * i + rowHeight * 0.5f;
                    float rowX = 10.0f + padding;

                    textRenderer.addTextLeftAligned(font, line, rowX, rowY, 0.24f);
                }
            }

            // --- Личный HUD (масса/рекорд), стиль и позиция как в JS ---
            if (!ownedIds.empty())
            {
                float screenW = static_cast<float>(window.width());
                float screenH = static_cast<float>(window.height());

                auto firstRsIt = renderStates.find(ownedIds[0]);

                float firstX = (firstRsIt != renderStates.end()) ? firstRsIt->second.x : 0.0f;
                float firstY = (firstRsIt != renderStates.end()) ? firstRsIt->second.y : 0.0f;

                std::ostringstream hudStream;
                hudStream << "x:" << static_cast<int>(firstX)
                    << " y:" << static_cast<int>(firstY)
                    << ". Your best: " << static_cast<int>(bestScoreSoFar)
                    << ". Now: " << static_cast<int>(currentScore)
                    << " (" << ownedIds.size() << ")";

                std::string hudText = hudStream.str();

                constexpr float hudFontScale = 20.0f / 70.0f; // шрифт ~20px, как SVGPlotFunction(20, ...) в JS

                float textWidthPx = font.measureWidth(hudText) * hudFontScale;
                float panelWidth = textWidthPx + 10.0f;
                float panelHeight = 34.0f;

                float panelCenterX = 10.0f + panelWidth * 0.5f;
                float panelCenterY = screenH - 10.0f - 22.0f - 10.0f + panelHeight * 0.5f;

                uiPanel.draw(
                    panelCenterX, panelCenterY,
                    panelWidth, panelHeight,
                    0.0f,
                    rectR, rectG, rectB, rectA,
                    0.0f, 0.0f, 0.0f, 0.0f,
                    0.0f,
                    screenW, screenH
                );

                textRenderer.addTextLeftAligned(font, hudText, 15.0f, panelCenterY, hudFontScale);
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
        }
        else
        {
            glClearColor(0.05f, 0.05f, 0.07f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);

            if (input.menuDownPressed && !serverList.empty())
            {
                selectedServerIndex = std::min(
                    selectedServerIndex + 1,
                    static_cast<int>(serverList.size()) - 1
                );
            }

            if (input.menuUpPressed)
            {
                selectedServerIndex = std::max(selectedServerIndex - 1, 0);
            }

            constexpr int visibleRows = 15;

            if (selectedServerIndex < menuScrollOffset)
                menuScrollOffset = selectedServerIndex;

            if (selectedServerIndex >= menuScrollOffset + visibleRows)
                menuScrollOffset = selectedServerIndex - visibleRows + 1;

            float screenW = static_cast<float>(window.width());
            float screenH = static_cast<float>(window.height());

            constexpr float rowHeight = 30.0f;
            constexpr float listWidth = 500.0f;

            float listX = screenW * 0.5f;
            float listTop = 60.0f;

            constexpr float dialogWidth = 500.0f;
            constexpr float dialogHeight = 600.0f;

            float dialogX = screenW * 0.5f;
            float dialogY = screenH * 0.5f;

            const float dialogLeft =
                dialogX - dialogWidth * 0.5f;

            const float dialogTop =
                dialogY - dialogHeight * 0.5f;

            constexpr float modeButtonWidth = 130.0f;
            constexpr float modeButtonHeight = 28.0f;
            constexpr float modeButtonGap = 3.0f;

            constexpr float modeIconSize = 26.0f;
            constexpr float modeIconGap = 6.0f;

            const float modesLeftX =
                dialogLeft + 110.0f;

            const float modesTopY =
                dialogTop + 185.0f;

            uiPanel.drawRoundedCorners(
                dialogX,
                dialogY,
                dialogWidth,
                dialogHeight,

                250.0f,
                250.0f,
                50.0f,
                50.0f,

                1.0f, 1.0f, 1.0f, 1.0f,
                0.0f, 0.0f, 0.0f, 0.0f,
                0.0f,

                screenW,
                screenH
            );

            textRenderer.begin();

            textRenderer.addText(
                menuFont,
                "PetriDish",
                screenW * 0.5f,
                dialogY - dialogHeight * 0.5f + 65.0f,
                0.55f
            );

            textRenderer.addText(
                menuFont,
                "Total players online: 1134",
                screenW * 0.5f,
                dialogY - dialogHeight * 0.5f + 105.0f,
                0.22f
            );

            textRenderer.end(
                menuFont,
                screenW,
                screenH,
                66.0f / 255.0f,
                139.0f / 255.0f,
                202.0f / 255.0f
            );

            for (size_t i = 0; i < gameModes.size(); ++i)
            {
                const float x = modesLeftX;

                const float y =
                    modesTopY +
                    static_cast<float>(i) *
                    (modeButtonHeight + modeButtonGap);

                const bool selected =
                    static_cast<int>(i) == selectedModeIndex;

                uiPanel.drawRoundedCorners(
                    x, y,
                    modeButtonWidth, modeButtonHeight,
                    5.0f, 5.0f, 5.0f, 5.0f,
                    selected ? 0.3608f : 0.2588f,
                    selected ? 0.7216f : 0.5451f,
                    selected ? 0.3608f : 0.7922f,
                    1.0f,
                    selected ? 0.2980f : 0.2078f,
                    selected ? 0.6824f : 0.4941f,
                    selected ? 0.2980f : 0.7412f,
                    1.0f,
                    1.0f,
                    screenW, screenH
                );
            }

            for (size_t i = 0; i < gameModes.size(); ++i)
            {
                const float x = modesLeftX;

                const float y =
                    modesTopY +
                    static_cast<float>(i) *
                    (modeButtonHeight + modeButtonGap);

                GLuint iconTexture = gameModeIconTextures[i];

                if (iconTexture == 0)
                    continue;

                const float buttonLeftEdge =
                    x - modeButtonWidth * 0.5f;

                const float iconCenterX =
                    buttonLeftEdge
                    - modeIconGap
                    - modeIconSize * 0.5f;

                iconRenderer.draw(
                    iconTexture,
                    iconCenterX,
                    y,
                    modeIconSize,
                    modeIconSize,
                    screenW,
                    screenH
                );
            }

            textRenderer.begin();

            for (size_t i = 0; i < gameModes.size(); ++i)
            {
                const float x = modesLeftX;

                const float y =
                    modesTopY +
                    static_cast<float>(i) *
                    (modeButtonHeight + modeButtonGap);

                textRenderer.addText(
                    gmFont,
                    gameModes[i].name,
                    x,
                    y,
                    0.18f
                );
            }

            textRenderer.end(
                gmFont,
                screenW,
                screenH,
                1.0f,
                1.0f,
                1.0f
            );

            if (input.menuConfirmPressed && !serverList.empty())
            {
                const auto& chosen = serverList[selectedServerIndex];

                size_t colonPos = chosen.address.find(':');

                if (colonPos != std::string::npos)
                {
                    std::string host = chosen.address.substr(0, colonPos);
                    int port = std::stoi(chosen.address.substr(colonPos + 1));

                    // ⚠️ Порт для wss = порт из списка минус 100, по логике JS
                    // (sport -= 100 для защищённого протокола). Не проверено на
                    // практике для ЭТОГО конкретного списка — если подключение
                    // не удастся, возможно, порт вообще не нужен (как в вашем
                    // текущем рабочем адресе без порта), тогда просто уберите
                    // ":" + port из url ниже.
                    std::string url = "wss://" + host + ":443";

                    world.reset();
                    network.connect(url);
                    appState = AppState::Playing;
                }
            }
            }
        window.swap();

        frameLimiter.endFrame();

        stats.endFrame(inputManager.mouseEventsThisFrame());

        if (stats.hasNewStats())
        {
            std::ostringstream title;
            title << " | FPS limit: " << frameLimiter.modeName();

            if (frameLimiter.mode() != FrameLimitMode::Unlimited)
            {
                title << " (" << frameLimiter.targetFps() << ")";
            }

            window.setTitle(title.str());
        }
    }

    ix::uninitNetSystem();

    return 0;
}