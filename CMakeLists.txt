cmake_minimum_required(VERSION 3.20)

project(AgarClient
    VERSION 0.1
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(SDL3 CONFIG REQUIRED)

find_package(glad CONFIG REQUIRED)

find_package(ixwebsocket CONFIG REQUIRED)

find_package(nlohmann_json CONFIG REQUIRED)

find_package(Stb REQUIRED)

find_package(harfbuzz CONFIG REQUIRED)

add_executable(AgarClient
    src/main.cpp
    src/Graphics/Window.cpp
    src/Graphics/Shader.cpp
    src/Graphics/Mesh.cpp
    src/Graphics/CircleMesh.cpp
    src/Graphics/PlayerColors.cpp
    src/Graphics/Camera.cpp
    src/Graphics/Font.cpp
    src/Graphics/TextRenderer.cpp
    src/Graphics/SkinManager.cpp
    src/Graphics/StbImage.cpp
    src/Graphics/SkinMesh.cpp
    src/Graphics/InstancedCircleRenderer.cpp
    src/Graphics/UIPanel.cpp
    src/Graphics/IconRenderer.cpp
    src/Graphics/GameRenderer.cpp
    src/Graphics/GameHUD.cpp

    src/UI/ModeIconManager.cpp
    src/UI/MainMenu.cpp
    src/UI/HUD/LeaderboardHUD.cpp

    src/Input/InputManager.cpp

    src/Core/FrameStats.cpp
    src/Core/FrameLimiter.cpp

    src/Network/NetworkClient.cpp
    src/Network/PacketHandler.cpp
    src/Network/ServerListFetcher.cpp
    src/Network/RealtimeInfoClient.cpp

    src/Game/World.cpp
    src/Game/ChatMessage.h
    src/Game/AimController.cpp
    src/Game/PlayerTracker.cpp
    src/Game/GameUpdater.cpp
 "src/UI/HUD/PlayerHUD.h" "src/UI/HUD/PlayerHUD.cpp" "src/UI/HUD/ChatHUD.h" "src/UI/HUD/ChatHUD.cpp")

target_link_libraries(AgarClient PRIVATE
    SDL3::SDL3
    glad::glad
    opengl32
    ixwebsocket::ixwebsocket
    bcrypt
    winhttp
)

target_include_directories(AgarClient PRIVATE ${Stb_INCLUDE_DIR})