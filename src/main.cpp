#define NOMINMAX

// SDL / OpenGL
#include <glad/glad.h>
#include <SDL3/SDL.h>

// C++ standard library
#include <algorithm>
#include <chrono>
#include <iostream>
#include <sstream>
#include <unordered_map>

// Core
#include "Core/FrameStats.h"
#include "Core/FrameLimiter.h"
#include "Core/GameModes.h"

// Graphics
#include "Graphics/Shader.h"
#include "Graphics/Window.h"
#include "Graphics/CircleMesh.h"
#include "Graphics/Camera.h"
#include "Graphics/Font.h"
#include "Graphics/TextRenderer.h"
#include "Graphics/SkinManager.h"
#include "Graphics/SkinMesh.h"
#include "Graphics/InstancedCircleRenderer.h"
#include "Graphics/UIPanel.h"
#include "Graphics/IconRenderer.h"
#include "Graphics/GameRenderer.h"
#include "Graphics/GameHUD.h"

// Graphics shaders
#include "Graphics/Shaders/CircleShader.h"
#include "Graphics/Shaders/TextShader.h"
#include "Graphics/Shaders/SkinShader.h"
#include "Graphics/Shaders/CircleInstancedShader.h"

// Input
#include "Input/InputManager.h"
#include "Input/InputState.h"

// Network
#include "Network/NetworkClient.h"
#include "Network/PacketHandler.h"
#include "Network/ServerListFetcher.h"

// UI
#include "UI/MenuState.h"
#include "UI/MainMenu.h"

// Game
#include "Game/World.h"
#include "Game/RenderState.h"
#include "Game/GameState.h"
#include "Game/AimController.h"
#include "Game/PlayerTracker.h"
#include "Game/GameUpdater.h"

// ixWebSocket
#include <ixwebsocket/IXNetSystem.h>

enum class AppState
{
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

    // -------------------------
    // Shaders
    // -------------------------

    Shader circleShader(
        CircleShader::vertex,
        CircleShader::fragment
    );

    Shader skinShader(
        SkinShader::vertex,
        SkinShader::fragment
    );

    Shader textShader(
        TextShader::vertex,
        TextShader::fragment
    );

    Shader circleInstancedShader(
        CircleInstancedShader::vertex,
        CircleInstancedShader::fragment
    );

    // -------------------------
    // Fonts
    // -------------------------

    Font font(
        "C:/dev/AgarClient/assets/fonts/arial.otf",
        70.0f,
        1.0f
    );

    Font menuFont(
        "C:/dev/AgarClient/assets/fonts/arial.otf",
        85.0f,
        0
    );

    Font gmFont(
        "C:/dev/AgarClient/assets/fonts/arial.otf",
        100.0f,
        0
    );


    // -------------------------
    // UI
    // -------------------------

    UIPanel uiPanel;
    IconRenderer iconRenderer;
    TextRenderer textRenderer(textShader);

    MainMenu mainMenu(
        uiPanel,
        textRenderer,
        iconRenderer,
        gmFont,
        menuFont
    );

    // -------------------------
    // Game meshes / renderers
    // -------------------------

    CircleMesh circleMesh;
    SkinMesh skinMesh;
    InstancedCircleRenderer foodRenderer;

    // -------------------------
    // Camera / frame limiter
    // -------------------------

    Camera camera;
    FrameLimiter frameLimiter(window.refreshRate());

    // -------------------------
    // Game
    // -------------------------

    World world;
    SkinManager skinManager;

    GameRenderer gameRenderer(
        circleShader,
        skinShader,
        circleInstancedShader,
        circleMesh,
        skinMesh,
        foodRenderer,
        skinManager,
        textRenderer,
        font,
        camera
    );

    // -------------------------
    // Mode icons
    // -------------------------

    std::vector<GLuint> gameModeIconTextures(
        gameModes.size(),
        0
    );

    for (size_t i = 0; i < gameModes.size(); ++i)
    {
        gameModeIconTextures[i] = iconRenderer.loadTexture(
            "C:/dev/AgarClient/" + gameModes[i].iconPath
        );
    }
    
    // -------------------------
    // Render state
    // -------------------------

    std::unordered_map<uint32_t, RenderState> renderStates;

    // -------------------------
    // Network
    // -------------------------

    PacketHandler packetHandler(9, world);

    NetworkClient network(packetHandler);

    packetHandler.setNetworkClient(network);

    network.setPlayerPassword("");
    network.setNickname("Android Player");
    network.setPlayerColor(6);
    

    InputManager inputManager;
    InputState input;
    FrameStats stats;
    GameState gameState;

    GameHUD gameHud(
        uiPanel,
        textRenderer,
        font,
        camera,
        world,
        gameState
    );

    AimController aimController(
        camera,
        network,
        gameState
    );

    PlayerTracker playerTracker(
        camera,
        gameState
    );

    GameUpdater gameUpdater(
        camera,
        network,
        skinManager,
        world,
        gameState
    );

    bool running = true;

    auto serverList = ServerListFetcher::fetch();

    AppState appState = AppState::SelectingServer;

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

            if ((input.splitRequested || input.ejectMassRequested) && !ownedIds.empty())
            {
                aimController.trySendFreshAim(
                    input,
                    static_cast<float>(window.width()),
                    static_cast<float>(window.height()),
                    true
                );

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

                if (nowMacro - gameState.lastMacroShotTime >= std::chrono::milliseconds(40))
                {
                    gameState.lastMacroShotTime = nowMacro;

                    aimController.trySendFreshAim(
                        input,
                        static_cast<float>(window.width()),
                        static_cast<float>(window.height()),
                        true
                    );
                    network.requestEjectMass();
                }
            }

            playerTracker.update(
                blobs,
                ownedIds,
                renderStates
            );

            gameUpdater.update(
                input,
                ownedIds,
                static_cast<float>(stats.deltaTime()),
                static_cast<float>(window.width()),
                static_cast<float>(window.height())
            );

            gameRenderer.draw(
                blobs,
                ownedIds,
                renderStates,
                static_cast<float>(window.width()),
                static_cast<float>(window.height())
            );

            gameHud.draw(
                ownedIds,
                renderStates,
                static_cast<float>(window.width()),
                static_cast<float>(window.height())
            );

            glBindTexture(GL_TEXTURE_2D, 0);
        }
        else
        {
            glClearColor(0.05f, 0.05f, 0.07f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);

            mainMenu.update(serverList);

            if (input.menuDownPressed)
            {
                mainMenu.moveSelectionDown(serverList);
            }

            if (input.menuUpPressed)
            {
                mainMenu.moveSelectionUp();
            }

            float screenW = static_cast<float>(window.width());
            float screenH = static_cast<float>(window.height());

            mainMenu.draw(
                serverList,
                gameModeIconTextures,
                screenW,
                screenH
            );

            if (mainMenu.hasConfirmedSelection(input, serverList))
            {
                std::string url =
                    mainMenu.selectedServerUrl(serverList);

                if (!url.empty())
                {
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