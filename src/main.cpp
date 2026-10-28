// SDL / OpenGL
#include <glad/glad.h>
#include <SDL3/SDL.h>

// C++ standard library
#include <algorithm>
#include <chrono>
#include <iostream>
#include <sstream>
#include <cmath>
#include <utility>
#include <unordered_map>
#include <exception>
#include <filesystem>
#include <string>
#include <iomanip>

// Core
#include "Core/FrameStats.h"
#include "Core/FrameLimiter.h"
#include "Core/GameModes.h"
#include "Core/AssetPath.h"

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
#include "Network/UserSkinDatabase.h"

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
#include "Game/MacroController.h"
#include "Game/ServerListEntry.h"

#include "Fonts/UiFonts.h"
// ixWebSocket
#include <ixwebsocket/IXNetSystem.h>

#include "VirusGuard/VirusGuard.h"
#include "VirusGuard/VirusEjector.h"

#include "FrameProfiler.h"

enum class AppState
{
    SelectingServer,
    Playing
};

const ServerListEntry* findConnectedServer(
    const std::vector<ServerListEntry>& serverList,
    const std::string& currentUrl
)
{
    if (currentUrl.empty())
        return nullptr;

    for (const auto& server : serverList)
    {
        const char* gateway = nullptr;

        switch (server.dataLoc)
        {
        case 1:
            gateway = "game900";
            break;

        case 2:
            gateway = "game903";
            break;

        case 3:
            gateway = "game906";
            break;

        default:
            continue;
        }

        const std::string serverUrl =
            std::string("ws://") +
            gateway +
            ".petridish.pw/" +
            server.id;

        if (serverUrl == currentUrl)
            return &server;
    }

    return nullptr;
}

static void showFatalError(const std::string& message)
{
    std::cerr << "Fatal error: " << message << '\n';
    SDL_ShowSimpleMessageBox(
        SDL_MESSAGEBOX_ERROR,
        "AgarClient",
        message.c_str(),
        nullptr
    );
}

struct NetSystemGuard
{
    NetSystemGuard() { ix::initNetSystem(); }
    ~NetSystemGuard() { ix::uninitNetSystem(); }
};

static int runGame()
{
    NetSystemGuard netGuard;

    Window window("AgarClient", 1100, 620);

    if (!window.isValid())
    {
        return 1;
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    if (!std::filesystem::exists(AssetPath::get("fonts/arial.otf")))
    {
        showFatalError(
            "Assets folder not found.\nExpected: " +
            (AssetPath::root() / "assets").string()
        );
        return 1;
    }

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
        AssetPath::get("fonts/arial.otf"),
        70.0f,
        1.0f
    );

    Font gmFont(
        AssetPath::get("fonts/movavi.otf"),
        100.0f,
        0
    );

    // Все UI-шрифты и UiTextRenderer собраны в одном объекте.
    UiFonts uiFonts;

    // -------------------------
    // Ресурсы предпросмотра скина
    // -------------------------

    World world;
    SkinManager skinManager;
    UserSkinDatabase userSkinDatabase;

    SkinMesh skinMesh;


    // -------------------------
    // UI
    // -------------------------

    UIPanel uiPanel;
    IconRenderer iconRenderer;
    TextRenderer textRenderer(textShader);

    MainMenu mainMenu(
        uiPanel,
        uiFonts,
        iconRenderer,
        skinShader,
        skinMesh,
        skinManager,
        userSkinDatabase
    );

    bool nicknameTextInputStarted = false;

    auto syncNicknameTextInput = [&]()
        {
            const bool shouldBeActive =
                mainMenu.isTextInputActive();

            if (shouldBeActive == nicknameTextInputStarted)
                return;

            if (shouldBeActive)
            {
                if (SDL_StartTextInput(window.handle()))
                    nicknameTextInputStarted = true;
            }
            else
            {
                SDL_StopTextInput(window.handle());
                nicknameTextInputStarted = false;
            }
        };

    // -------------------------
    // Game meshes / renderers
    // -------------------------

    CircleMesh circleMesh;
    InstancedCircleRenderer foodRenderer;

    // -------------------------
    // Camera / frame limiter
    // -------------------------

    Camera camera;
    FrameLimiter frameLimiter;

    // -------------------------
    // Game
    // -------------------------

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
        camera,
        userSkinDatabase
    );

    VirusGuard virusGuard(world);

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
            AssetPath::fromRoot(gameModes[i].iconPath)
        );
    }

    GLuint blueButtonTexture =
        iconRenderer.loadTexture(AssetPath::get("icons/blueButton.png"));

    GLuint redButtonTexture =
        iconRenderer.loadTexture(AssetPath::get("icons/redButton.png"));

    GLuint yellowButtonTexture =
        iconRenderer.loadTexture(AssetPath::get("icons/yellowButton.png"));
    
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

    // VirusEjector: first run is deliberately diagnostic-only.
    virus_ejector::Config virusEjectorConfig;
    virusEjectorConfig.autoFire = false;

    virus_ejector::VirusEjector virusEjector(
        virusEjectorConfig,
        [&network](virus_ejector::Vec2 target)
        {
            // Match the normal client's integer world-coordinate aim.
            network.sendAimPosition(std::floor(target.x), std::floor(target.y));
        },
        [&network, &virusGuard]()
        {
            network.requestEjectMass();
            virusGuard.noteOwnEject();
        }
    );

    // World::snapshot() reuses its shared_ptr until the world changes.
    uint64_t previousVirusEjectorWorldVersion = 0;
    double virusEjectorSnapshotArrival = 0.0;
    bool hasVirusEjectorSnapshotArrival = false;

    // Rate-limit repeated diagnostics for a stable threat.
    uint32_t loggedVirusId = 0;
    uint32_t loggedCellId = 0;
    bool hadLoggedVirusThreat = false;
    auto lastVirusThreatLog = std::chrono::steady_clock::time_point{};

    InputManager inputManager;
    InputState input;
    FrameStats stats;
    GameState gameState;
    FrameProfiler profiler;
    GameHUD gameHud(
        uiPanel,
        textRenderer,
        font,
        camera,
        world,
        gameState,
        chatSkinShader,
        skinMesh,
        skinManager,
        uiFonts,
        userSkinDatabase
    );

    AimController aimController(
        camera,
        network,
        gameState
    );

    MacroController macroController(network);

    PlayerTracker playerTracker(
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

    // --------------------------------------------------------
    // ColorPicker — обновление цвета и закрытие меню
    // --------------------------------------------------------

    mainMenu.setOnPlayerColorSelected(
        [&network, &world, &menuOpen](int colorIndex)
        {
            // Сохраняем выбранный цвет.
            network.setPlayerColor(
                static_cast<uint8_t>(colorIndex)
            );

            // Отправляем цвет, только если WebSocket открыт.
            // Проверка находится внутри sendPlayerColor().
            network.sendPlayerColor();

            // Во время игры закрываем всё меню.
            const auto ownedIds = world.getOwnedIds();

            if (!ownedIds.empty())
            {
                menuOpen = false;
            }
        }
    );

    auto serverList = ServerListFetcher::fetch();

    // Realtime online серверов — обновляет тот же serverList по ссылке,
    // второй список не создаётся. Подключаемся сразу и держим соединение
    // всё время жизни приложения (переподключение — забота самого класса).
    RealtimeInfoClient realtimeInfoClient;
    realtimeInfoClient.setServerList(serverList);
    realtimeInfoClient.connect();

    AppState appState = AppState::SelectingServer;

    auto enterPlaying = [&]()
        {
            camera.onShow();
            appState = AppState::Playing;
        };

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

            const std::string nickname = mainMenu.nickname();

            const int selectedModeIndex =
                mainMenu.state().selectedModeIndex;

            const bool isClanhouseSelected =
                selectedModeIndex >= 0 &&
                selectedModeIndex < static_cast<int>(gameModes.size()) &&
                gameModes[selectedModeIndex].id == "CLANHOUSE";

            const std::string serverPass =
                isClanhouseSelected
                ? mainMenu.chPassword()
                : "";

            network.setNickname(nickname);
            network.setPlayerPassword(mainMenu.playerPassword());
            network.setPlayerColor(
                static_cast<uint8_t>(
                    mainMenu.selectedColorIndex()
                    )
            );

            const ConnectionMode connectionMode =
                mainMenu.state().joinMode ==
                MenuState::JoinMode::Play
                ? ConnectionMode::Play
                : ConnectionMode::Spectate;

            camera.spectator =
                connectionMode == ConnectionMode::Spectate;

            if (!network.isConnected())
            {
                hadOwnedCells = false;
                gameState.mapCentered = false;
                world.reset();
                gameState.bestScoreSoFar = 0.0f;

                // На случай, если предыдущее подключение
                // завершилось ошибкой (например, HTTP 403).
                network.disconnect();

                network.connect(
                    url,
                    serverPass,
                    connectionMode
                );

                canRespawn = false;
                enterPlaying();
            }
            else if (url == network.currentUrl())
            {
                // Случай 2: тот же сервер.
                // Пока оставляем существующее соединение.
                // Для Play выполняем spawn, для Spectate
                // снова запрашиваем spectator.
                if (connectionMode == ConnectionMode::Play)
                {
                    camera.spectator = false;
                    network.requestSpawnFromSpectate();
                }
                else
                {
                    camera.spectator = true;
                    network.requestSpectate();
                }

                enterPlaying();
            }
            else
            {
                // Случай 3: переключение на другой сервер.
                network.disconnect();
                hadOwnedCells = false;
                gameState.mapCentered = false;
                world.reset();
                gameState.bestScoreSoFar = 0.0f;

                network.connect(
                    url,
                    serverPass,
                    connectionMode
                );

                enterPlaying();
            }

            menuOpen = false;
        };

    while (running)
    {
        stats.beginFrame();

        profiler.beginFrame();

        realtimeInfoClient.update();

        if (appState == AppState::Playing)
        {
            network.update();
            profiler.mark(FrameProfiler::Network);
        }

        input.textInputActive =
            mainMenu.isTextInputActive();

        running = inputManager.poll(input);

        // Обработка ввода Nick / Password / CH Password
        if (mainMenu.isTextInputActive())
        {
            // --------------------------------------------------------
            // Выделение и буфер обмена
            // --------------------------------------------------------

            if (input.textSelectAllPressed)
                mainMenu.selectAllText();

            if (input.textCopyPressed && mainMenu.hasTextSelection())
            {
                const std::string selected =
                    mainMenu.selectedText();

                SDL_SetClipboardText(selected.c_str());
            }

            if (input.textCutPressed && mainMenu.hasTextSelection())
            {
                const std::string selected =
                    mainMenu.selectedText();

                SDL_SetClipboardText(selected.c_str());

                mainMenu.deleteTextSelection();
            }

            if (input.textPastePressed)
            {
                char* clipboardText = SDL_GetClipboardText();

                if (clipboardText)
                {
                    mainMenu.appendTextInput(clipboardText);
                    SDL_free(clipboardText);
                }
            }

            // --------------------------------------------------------
            // Перемещение курсора
            // --------------------------------------------------------

            if (input.textLeftPressed)
                mainMenu.moveTextCursorLeft(input.textShiftHeld);

            if (input.textRightPressed)
                mainMenu.moveTextCursorRight(input.textShiftHeld);

            if (input.textHomePressed)
                mainMenu.moveTextCursorHome(input.textShiftHeld);

            if (input.textEndPressed)
                mainMenu.moveTextCursorEnd(input.textShiftHeld);

            // --------------------------------------------------------
            // Удаление
            // --------------------------------------------------------

            if (input.textBackspacePressed)
                mainMenu.backspaceTextInput();

            if (input.textDeletePressed)
                mainMenu.deleteTextForward();

            // --------------------------------------------------------
            // Ввод текста
            // --------------------------------------------------------

            if (!input.textInput.empty())
                mainMenu.appendTextInput(input.textInput);

            // --------------------------------------------------------
            // Завершение редактирования
            // --------------------------------------------------------

            if (input.textEnterPressed || input.textEscapePressed)
                mainMenu.finishTextInput();
        }

        window.updateSize();

        skinManager.processCompleted();

        const float screenW =
            static_cast<float>(window.width());

        const float screenH =
            static_cast<float>(window.height());

        camera.onResize(screenW, screenH);

        profiler.mark(FrameProfiler::Input);

        if (appState == AppState::Playing)
        {
            // --------------------------------------------------------
            // Меню по ESC
            // --------------------------------------------------------

            if (input.menuTogglePressed)
            {
                menuOpen = !menuOpen;
            }


            if (network.consumeConnectionFailed())
            {
                std::cout << "Connection failed, reopening server menu.\n";
                menuOpen = true;
            }

            auto blobs = world.snapshot();
            const uint64_t blobsVersion = world.version();
            const std::uint64_t foodVersion = world.foodVersion();

            auto ownedIds = world.getOwnedIds();

            // --------------------------------------------------------
            // Игровой ввод
            // --------------------------------------------------------

            if (input.fullscreenPressed)
            {
                window.toggleFullscreen();
            }

            if (!menuOpen)
            {
                // E-макрос обрабатывается первым.
                macroController.update(
                    input,
                    blobs,
                    ownedIds,
                    camera,
                    screenW,
                    screenH
                );

                macroController.setQHeld(
                    input.ejectMassKeyHeld
                );

                // Обычный aim работает только когда E не зажат.
                if (
                    !ownedIds.empty() &&
                    !input.action(GameAction::MacroE).held
                    )
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
                    camera.spectator = false;
                    network.requestSpawnFromSpectate();
                }

                // Split / одиночный Eject
                if (
                    (input.splitRequested ||
                        input.ejectMassRequested) &&
                    !ownedIds.empty()
                    )
                {
                    const bool macroHeld =
                        input.action(GameAction::MacroE).held;

                    if (!macroHeld)
                    {
                        aimController.trySendFreshAim(
                            input,
                            screenW,
                            screenH,
                            true
                        );
                    }

                    if (input.splitRequested)
                    {
                        if (macroHeld)
                        {
                            macroController.requestManualSplit(
                                input,
                                camera,
                                screenW,
                                screenH
                            );
                        }
                        else
                        {
                            network.requestSplit();
                        }
                    }

                    if (input.ejectMassRequested)
                    {
                        if (macroHeld)
                        {
                            macroController.requestManualEject(
                                input,
                                camera,
                                screenW,
                                screenH
                            );
                        }
                        else
                        {
                            network.requestEjectMass();
                            virusGuard.noteOwnEject();
                        }
                    }
                }

                // Старый Q-макрос.
                // Во время E-макроса не перебиваем его aim.
                if (
                    input.ejectMassKeyHeld &&
                    !input.action(GameAction::MacroE).held &&
                    !ownedIds.empty()
                    )
                {
                    auto nowMacro =
                        std::chrono::steady_clock::now();

                    if (
                        nowMacro -
                        gameState.lastMacroShotTime >=
                        std::chrono::milliseconds(200)
                        )
                    {
                        gameState.lastMacroShotTime =
                            nowMacro;

                        aimController.trySendFreshAim(
                            input,
                            screenW,
                            screenH,
                            true
                        );

                        network.requestEjectMass();
                        network.requestEjectMass();
                        virusGuard.noteOwnEject();
                    }
                }
            }

            // --------------------------------------------------------
            // Обновление игры
            // --------------------------------------------------------

            const float frameDeltaTime =
                static_cast<float>(stats.deltaTime());

            const float segmentDuration = 0.12f;

            gameRenderer.updateRenderStates(
                blobs,
                renderStates,
                segmentDuration
            );

            gameRenderer.updatePositions(
                renderStates,
                frameDeltaTime
            );

            playerTracker.update(
                blobs,
                ownedIds,
                renderStates
            );

            gameUpdater.update(
                input,
                ownedIds,
                renderStates,
                frameDeltaTime,
                screenW,
                screenH,
                menuOpen
            );

            const bool hasOwnedCells =
                !ownedIds.empty();

            profiler.mark(FrameProfiler::Update);

            if (hadOwnedCells && !hasOwnedCells)
            {
                std::cout
                    << "Player died. Switching to spectate.\n";

                network.requestSpectate();
                camera.spectator = true;

                canRespawn = true;
                menuOpen = false;
            }

            hadOwnedCells = hasOwnedCells;

            // --------------------------------------------------------
            // Рендер игры
            // --------------------------------------------------------


            float targetWorldX;
            float targetWorldY;

            camera.screenToWorld(
                input.mouseX,
                input.mouseY,
                targetWorldX,
                targetWorldY
            );

            if (!menuOpen)
            {
                const auto virusEjectorNow = std::chrono::steady_clock::now();
                const double nowSeconds = std::chrono::duration<double>(
                    virusEjectorNow.time_since_epoch()
                ).count();

                // Keep the arrival timestamp stable while World returns the same snapshot.
                if (!hasVirusEjectorSnapshotArrival ||
                    blobsVersion != previousVirusEjectorWorldVersion)
                {
                    previousVirusEjectorWorldVersion = blobsVersion;
                    virusEjectorSnapshotArrival = nowSeconds;
                    hasVirusEjectorSnapshotArrival = true;
                }

                const auto toSeconds = [](std::chrono::steady_clock::time_point t)
                    {
                        return std::chrono::duration<double>(t.time_since_epoch()).count();
                    };

                std::vector<virus_ejector::Body> viruses;
                std::vector<virus_ejector::Body> ownCells;

                if (blobs)
                {
                    viruses.reserve(blobs->size() / 8 + 1);
                    for (const auto& [id, blob] : *blobs)
                    {
                        if (!blob.isVirus && blob.cellType != CellType::Virus)
                            continue;

                        viruses.push_back({
                            id,
                            { blob.targetX, blob.targetY },
                            blob.targetSize,
                            toSeconds(blob.lastUpdateTime)
                            });
                    }

                    ownCells.reserve(ownedIds.size());
                    for (const uint32_t id : ownedIds)
                    {
                        const auto blobIt = blobs->find(id);
                        if (blobIt == blobs->end())
                            continue;

                        const auto renderIt = renderStates.find(id);
                        if (renderIt != renderStates.end() && renderIt->second.initialized)
                        {
                            // Use the displayed/interpolated position for our cells.
                            ownCells.push_back({
                                id,
                                { renderIt->second.x, renderIt->second.y },
                                renderIt->second.size,
                                nowSeconds
                                });
                        }
                        else
                        {
                            // Fallback until a render state has been initialized.
                            const Blob& blob = blobIt->second;
                            ownCells.push_back({
                                id,
                                { blob.targetX, blob.targetY },
                                blob.targetSize,
                                toSeconds(blob.lastUpdateTime)
                                });
                        }
                    }
                }

                virusEjector.onSnapshot(
                    nowSeconds,
                    virusEjectorSnapshotArrival,
                    std::move(viruses),
                    std::move(ownCells)
                );
                virusEjector.onUserCursor({ targetWorldX, targetWorldY });

                // NetworkClient::pingMs() is an RTT measurement in milliseconds.
                const double rttSeconds =
                    static_cast<double>((std::max)(0, network.pingMs())) / 1000.0;

                virusEjector.update(nowSeconds, rttSeconds);

                const auto& threat = virusEjector.latestThreat();
                if (threat)
                {
                    const bool changed =
                        !hadLoggedVirusThreat ||
                        threat->virusId != loggedVirusId ||
                        threat->cellId != loggedCellId;
                    const bool periodic =
                        lastVirusThreatLog == std::chrono::steady_clock::time_point{} ||
                        virusEjectorNow - lastVirusThreatLog >= std::chrono::milliseconds(100);

                    if (changed || periodic)
                    {
                        const auto oldFlags = std::cout.flags();
                        const auto oldPrecision = std::cout.precision();
                        std::cout << std::fixed << std::setprecision(3)
                            << "[VirusEjector] t=" << nowSeconds
                            << " virus=" << threat->virusId
                            << " cell=" << threat->cellId
                            << " tti=" << threat->tti << "s"
                            << " tau=" << threat->tau << "s"
                            << " slack=" << (threat->tti - threat->tau) << "s"
                            << " aim=(" << threat->aim.x << ", "
                            << threat->aim.y << ")"
                            << " rtt=" << rttSeconds << "s\n";
                        std::cout.flags(oldFlags);
                        std::cout.precision(oldPrecision);

                        loggedVirusId = threat->virusId;
                        loggedCellId = threat->cellId;
                        lastVirusThreatLog = virusEjectorNow;
                    }
                    hadLoggedVirusThreat = true;
                }
                else if (hadLoggedVirusThreat)
                {
                    std::cout << "[VirusEjector] threat cleared\n";
                    hadLoggedVirusThreat = false;
                }
            }

            {
                FrameProfiler::ScopedTimer timer(
                    profiler,
                    FrameProfiler::GameRender
                );

                gameRenderer.draw(
                    blobs,
                    foodVersion,
                    ownedIds,
                    renderStates,
                    screenW,
                    screenH,
                    targetWorldX,
                    targetWorldY,
                    profiler,
                    &virusGuard,
                    input.splitPreviewHeld
                );
            }

            std::string serverInfoText;

            if (const ServerListEntry* server =
                findConnectedServer(serverList, network.currentUrl()))
            {
                const int realtimeOnline = network.serverPlayersOnline();

                const int online =
                    realtimeOnline >= 0
                    ? realtimeOnline
                    : server->online;

                const int uptime = network.serverUptimeMinutes();
                const int totalMass = network.serverTotalMass();
                const int totalCells = network.serverTotalCells();

                std::ostringstream stream;

                stream << server->mode;

                if (!server->modenumber.empty())
                    stream << " #" << server->modenumber;

                if (!server->sname.empty())
                    stream << ' ' << server->sname;

                stream
                    << ' ' << online
                    << '/' << server->connectlimit
                    << " online (uptime: ";

                if (uptime >= 0)
                    stream << uptime;
                else
                    stream << '?';

                stream << " min) / totalmass: ";

                if (totalMass >= 0)
                    stream << totalMass;
                else
                    stream << '?';

                stream << " / bombs: ";

                if (totalCells >= 0)
                    stream << totalCells;
                else
                    stream << '?';

                serverInfoText = stream.str();
            }

            {
                FrameProfiler::ScopedTimer timer(
                    profiler,
                    FrameProfiler::HUD
                );

                gameHud.draw(
                    profiler,
                    ownedIds,
                    renderStates,
                    screenW,
                    screenH,
                    stats.fps(),
                    network.pingMs(),
                    network.connectedDurationSeconds(),
                    serverInfoText,
                    network.gameMode()
                );
            }

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
                        const std::string selectedUrl =
                            mainMenu.selectedServerUrl(serverList);

                        network.setNickname(mainMenu.nickname());
                        network.setPlayerPassword(mainMenu.playerPassword());
                        network.setPlayerColor(
                            static_cast<uint8_t>(
                                mainMenu.selectedColorIndex()
                                )
                        );

                        const bool respawnOnCurrentServer =
                            canRespawn &&
                            mainMenu.state().joinMode ==
                            MenuState::JoinMode::Play &&
                            network.isConnected() &&
                            selectedUrl == network.currentUrl();

                        if (respawnOnCurrentServer)
                        {
                            // После смерти повторно входим на тот же сервер.
                            network.requestSpawnFromSpectate();

                            canRespawn = false;
                            menuOpen = false;
                        }
                        else
                        {
                            // Новый сервер или новое подключение:
                            // обычный сценарий подключения.
                            connectToSelectedServer(selectedUrl);
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
                    input.mouseY,
                    false
                );
            }

            profiler.mark(FrameProfiler::Draw);

        }
        else
        {
            // --------------------------------------------------------
            // Экран выбора сервера
            // --------------------------------------------------------

            glClearColor(
                242.0f / 255.0f,
                251.0f / 255.0f,
                255.0f / 255.0f,
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
                input.mouseY,
                true
            );

            profiler.mark(FrameProfiler::Draw);

            if (mainMenu.hasConfirmedSelection(input, serverList))
            {
                connectToSelectedServer(
                    mainMenu.selectedServerUrl(serverList)
                );
            }
        }

        syncNicknameTextInput();
        window.swap();
        profiler.mark(FrameProfiler::Swap);

        frameLimiter.endFrame();
        profiler.mark(FrameProfiler::Wait);
        profiler.endFrame();

        stats.endFrame(
            inputManager.mouseEventsThisFrame()
        );

        if (stats.hasNewStats())
        {
            std::ostringstream out;
            out << std::fixed << std::setprecision(3);

            const auto printStat =
                [&](FrameProfiler::Stage stage)
                {
                    const auto s = profiler.stat(stage);

                    out << FrameProfiler::stageName(stage)
                        << ' '
                        << s.p50 << '/'
                        << s.p99 << '/'
                        << s.max
                        << " ms  ";
                };

            printStat(FrameProfiler::Input);
            printStat(FrameProfiler::Network);
            printStat(FrameProfiler::Update);
            printStat(FrameProfiler::Draw);
            printStat(FrameProfiler::Swap);
            printStat(FrameProfiler::Wait);
            printStat(FrameProfiler::FrameTotal);
            printStat(FrameProfiler::GameRender);
            printStat(FrameProfiler::GameRenderSplitSimulate);
            printStat(FrameProfiler::GameRenderPrepare);
            printStat(FrameProfiler::GameRenderSort);
            printStat(FrameProfiler::GameRenderSkinLookup);
            printStat(FrameProfiler::GameRenderShaderSetup);
            printStat(FrameProfiler::GameRenderInstanced);
            printStat(FrameProfiler::GameRenderEntities);
            printStat(FrameProfiler::GameRenderSplitPreview);
            printStat(FrameProfiler::GameRenderTextFlush);
            printStat(FrameProfiler::GameRenderEntityOutlines);
            printStat(FrameProfiler::GameRenderSkinDraw);
            printStat(FrameProfiler::GameRenderTextBuild);
            printStat(FrameProfiler::HUD);
            printStat(FrameProfiler::Leaderboard);
            printStat(FrameProfiler::Chat);
            printStat(FrameProfiler::ChatPrepare);
            printStat(FrameProfiler::ChatVisibility);
            printStat(FrameProfiler::ChatRender);
            printStat(FrameProfiler::ChatTextBuild);
            printStat(FrameProfiler::ChatTextFlush);
            printStat(FrameProfiler::ChatVerifiedMarks);
            printStat(FrameProfiler::PlayerHUD);
            printStat(FrameProfiler::TextEnd);

        }
    }

    return 0;
}



int main()
{
    try
    {
        return runGame();
    }
    catch (const std::exception& e)
    {
        showFatalError(e.what());
        return 1;
    }
}