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
#include "Graphics/Shaders/ChatSkinShader.h"
#include "Graphics/Shaders/CircleInstancedShader.h"

// Input
#include "Input/InputManager.h"
#include "Input/InputState.h"

// Network
#include "Network/NetworkClient.h"
#include "Network/PacketHandler.h"
#include "Network/ServerListFetcher.h"
#include "Network/RealtimeInfoClient.h"

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

    Shader chatSkinShader(
        ChatSkinShader::vertex,
        ChatSkinShader::fragment
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

    Font gmFont(
        "C:/dev/AgarClient/assets/fonts/movavi.otf",
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
        font
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

    GLuint blueButtonTexture =
        iconRenderer.loadTexture(
            "C:/dev/AgarClient/assets/icons/blueButton.png"
        );

    GLuint redButtonTexture =
        iconRenderer.loadTexture(
            "C:/dev/AgarClient/assets/icons/redButton.png"
        );

    GLuint yellowButtonTexture =
        iconRenderer.loadTexture(
            "C:/dev/AgarClient/assets/icons/yellowButton.png"
        );
    
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
        gameState,
        chatSkinShader,
        skinMesh,
        skinManager
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

    bool menuOpen = false;
    bool hadOwnedCells = false;
    bool canRespawn = false;

    auto serverList = ServerListFetcher::fetch();

    // Realtime online серверов — обновляет тот же serverList по ссылке,
    // второй список не создаётся. Подключаемся сразу и держим соединение
    // всё время жизни приложения (переподключение — забота самого класса).
    RealtimeInfoClient realtimeInfoClient;
    realtimeInfoClient.setServerList(serverList);
    realtimeInfoClient.connect();

    AppState appState = AppState::SelectingServer;

    // ------------------------------------------------------------
    // Клик "Play" в меню — три случая, как в JS-версии (connn()):
    //   1) сокета ещё нет                 -> открыть новый
    //   2) уже подключены к этому серверу -> сокет не трогаем, просто спавн
    //   3) подключены к другому серверу   -> закрыть старый, открыть новый
    // ------------------------------------------------------------
    auto connectToSelectedServer =
        [&](const std::string& url)
        {
            if (url.empty())
                return;

            const ConnectionMode connectionMode =
                mainMenu.state().joinMode ==
                MenuState::JoinMode::Play
                ? ConnectionMode::Play
                : ConnectionMode::Spectate;

            if (!network.isConnected())
            {
                // Случай 1: подключения ещё не было вовсе.
                hadOwnedCells = false;
                world.reset();

                network.connect(
                    url,
                    "",
                    connectionMode
                );

                appState = AppState::Playing;
            }
            else if (url == network.currentUrl())
            {
                // Случай 2: тот же сервер.
                // Пока оставляем существующее соединение.
                // Для Play выполняем spawn, для Spectate
                // снова запрашиваем spectator.
                if (connectionMode == ConnectionMode::Play)
                {
                    network.requestPlay();
                }
                else
                {
                    network.requestSpectate();
                }

                appState = AppState::Playing;
            }
            else
            {
                // Случай 3: переключение на другой сервер.
                network.disconnect();
                hadOwnedCells = false;
                world.reset();

                network.connect(
                    url,
                    "",
                    connectionMode
                );

                appState = AppState::Playing;
            }

            menuOpen = false;
        };

    while (running)
    {
        stats.beginFrame();

        realtimeInfoClient.update();

        frameLimiter.beginFrame();

        if (input.cycleFpsLimitPressed)
        {
            frameLimiter.cycleMode();
        }

        running = inputManager.poll(input);

        const float screenW =
            static_cast<float>(window.width());

        const float screenH =
            static_cast<float>(window.height());

        if (appState == AppState::Playing)
        {
            // --------------------------------------------------------
            // Меню по ESC
            // --------------------------------------------------------

            if (input.menuTogglePressed)
            {
                menuOpen = !menuOpen;
            }

            auto blobs = world.snapshot();
            auto ownedIds = world.getOwnedIds();

            // --------------------------------------------------------
            // Игровой ввод
            // --------------------------------------------------------

            if (!menuOpen)
            {
                if (!ownedIds.empty())
                {
                    aimController.trySendFreshAim(
                        input,
                        screenW,
                        screenH,
                        false
                    );
                }
                
                if (input.spawnRequestPressed)
                {
                    network.requestSpawn();
                }


                if (
                    (input.splitRequested ||
                        input.ejectMassRequested) &&
                    !ownedIds.empty()
                    )
                {
                    aimController.trySendFreshAim(
                        input,
                        screenW,
                        screenH,
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

                if (
                    input.ejectMassKeyHeld &&
                    !ownedIds.empty()
                    )
                {
                    auto nowMacro =
                        std::chrono::steady_clock::now();

                    if (
                        nowMacro -
                        gameState.lastFreshAimSendTime >=
                        std::chrono::milliseconds(5)
                        )
                    {
                        gameState.lastFreshAimSendTime =
                            nowMacro;

                        aimController.trySendFreshAim(
                            input,
                            screenW,
                            screenH,
                            true
                        );

                    }
                    if (
                        nowMacro -
                        gameState.lastMacroShotTime >=
                        std::chrono::milliseconds(200)
                        )
                    {
                        gameState.lastMacroShotTime =
                            nowMacro;
                        network.requestEjectMass();
                        network.requestEjectMass();
                    }
                }
            }

            // --------------------------------------------------------
            // Обновление игры
            // --------------------------------------------------------

            playerTracker.update(
                blobs,
                ownedIds,
                renderStates
            );

            gameUpdater.update(
                input,
                ownedIds,
                static_cast<float>(stats.deltaTime()),
                screenW,
                screenH,
                menuOpen
            );

            const auto currentOwnedIds =
                world.getOwnedIds();

            const bool hasOwnedCells =
                !currentOwnedIds.empty();

            if (hadOwnedCells && !hasOwnedCells)
            {
                std::cout
                    << "Player died. Switching to spectate.\n";

                network.requestSpectate();

                canRespawn = true;
                menuOpen = false;
            }

            hadOwnedCells = hasOwnedCells;

            // --------------------------------------------------------
            // Рендер игры
            // --------------------------------------------------------

            gameRenderer.draw(
                blobs,
                ownedIds,
                renderStates,
                screenW,
                screenH
            );

            gameHud.draw(
                ownedIds,
                renderStates,
                screenW,
                screenH,
                stats.fps()
            );

            glBindTexture(GL_TEXTURE_2D, 0);

            // --------------------------------------------------------
            // Меню поверх игры
            // --------------------------------------------------------

            if (menuOpen)
            {
                mainMenu.update(serverList);

                if (input.leftButtonJustPressed)
                {
                    mainMenu.handleMouseClick(
                        input.mouseX,
                        input.mouseY,
                        input.leftButtonDoubleClicked,
                        serverList,
                        screenW,
                        screenH
                    );
                }

                if (input.leftButtonJustReleased)
                {
                    mainMenu.handleMouseRelease(
                        input.mouseX,
                        input.mouseY,
                        screenW,
                        screenH
                    );

                    if (mainMenu.hasConfirmedSelection(input, serverList))
                    {
                        if (
                            canRespawn &&
                            mainMenu.state().joinMode ==
                            MenuState::JoinMode::Play
                            )
                        {
                            // После смерти: только повторный spawn.
                            network.requestSpawnFromSpectate();

                            canRespawn = false;
                            menuOpen = false;
                        }
                        else
                        {
                            connectToSelectedServer(
                                mainMenu.selectedServerUrl(serverList)
                            );
                        }
                    }
                }

                mainMenu.handleMouseDrag(
                    input.mouseX,
                    input.mouseY,
                    input.leftButton,
                    screenW,
                    screenH
                );

                if (input.menuDownPressed)
                {
                    mainMenu.moveSelectionDown(serverList);
                }

                if (input.menuUpPressed)
                {
                    mainMenu.moveSelectionUp();
                }

                mainMenu.handleMouseWheel(
                    input.mouseX,
                    input.mouseY,
                    input.mouseWheel,
                    screenW,
                    screenH
                );

                mainMenu.draw(
                    serverList,
                    gameModeIconTextures,
                    blueButtonTexture,
                    redButtonTexture,
                    yellowButtonTexture,
                    screenW,
                    screenH,
                    input.mouseX,
                    input.mouseY
                );
            }
        }
        else
        {
            // --------------------------------------------------------
            // Экран выбора сервера
            // --------------------------------------------------------

            glClearColor(
                0.05f,
                0.05f,
                0.07f,
                1.0f
            );

            glClear(GL_COLOR_BUFFER_BIT);

            mainMenu.update(serverList);

            if (input.leftButtonJustPressed)
            {
                mainMenu.handleMouseClick(
                    input.mouseX,
                    input.mouseY,
                    input.leftButtonDoubleClicked,
                    serverList,
                    screenW,
                    screenH
                );
            }

            if (input.leftButtonJustReleased)
            {
                mainMenu.handleMouseRelease(
                    input.mouseX,
                    input.mouseY,
                    screenW,
                    screenH
                );
            }

            mainMenu.handleMouseDrag(
                input.mouseX,
                input.mouseY,
                input.leftButton,
                screenW,
                screenH
            );

            if (input.menuDownPressed)
            {
                mainMenu.moveSelectionDown(serverList);
            }

            if (input.menuUpPressed)
            {
                mainMenu.moveSelectionUp();
            }

            mainMenu.handleMouseWheel(
                input.mouseX,
                input.mouseY,
                input.mouseWheel,
                screenW,
                screenH
            );

            mainMenu.draw(
                serverList,
                gameModeIconTextures,
                blueButtonTexture,
                redButtonTexture,
                yellowButtonTexture,
                screenW,
                screenH,
                input.mouseX,
                input.mouseY
            );

            if (mainMenu.hasConfirmedSelection(input, serverList))
            {
                connectToSelectedServer(
                    mainMenu.selectedServerUrl(serverList)
                );
            }
        }

        window.swap();

        frameLimiter.endFrame();

        stats.endFrame(
            inputManager.mouseEventsThisFrame()
        );

        if (stats.hasNewStats())
        {
            std::ostringstream title;

            title
                << " | FPS limit: "
                << frameLimiter.modeName();

            if (
                frameLimiter.mode() !=
                FrameLimitMode::Unlimited
                )
            {
                title
                    << " ("
                    << frameLimiter.targetFps()
                    << ")";
            }

            window.setTitle(title.str());
        }
    }

    ix::uninitNetSystem();

    return 0;
}