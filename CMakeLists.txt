cmake_minimum_required(VERSION 3.20)

project(AgarClient
    VERSION 0.1
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

find_package(SDL3 CONFIG REQUIRED)

find_package(glad CONFIG REQUIRED)

find_package(ixwebsocket CONFIG REQUIRED)

find_package(nlohmann_json CONFIG REQUIRED)

find_package(Stb REQUIRED)

find_package(harfbuzz CONFIG REQUIRED)
find_package(Freetype REQUIRED)

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
    src/Graphics/LineRenderer.cpp
    src/Graphics/BackgroundGridRenderer.cpp

    src/UI/ModeIconManager.cpp
    src/UI/MainMenu.cpp
    src/UI/HUD/LeaderboardHUD.cpp
    src/UI/ColorPickerPanel.cpp

    src/Input/InputManager.cpp

    src/Core/FrameStats.cpp
    src/Core/FrameLimiter.cpp

    src/Network/NetworkClient.cpp
    src/Network/PacketHandler.cpp
    src/Network/ServerListFetcher.cpp
    src/Network/RealtimeInfoClient.cpp
    src/Network/UserSkinDatabase.cpp

    src/Fonts/UiFont.cpp
    src/Fonts/UiTextRenderer.cpp

    src/Game/World.cpp
    src/Game/ChatMessage.h
    src/Game/AimController.cpp
    src/Game/PlayerTracker.cpp
    src/Game/GameUpdater.cpp
    src/Game/SplitPrediction.cpp
    src/VirusGuard/VirusGuard.cpp
 "src/UI/HUD/PlayerHUD.h" "src/UI/HUD/PlayerHUD.cpp" "src/UI/HUD/ChatHUD.h" "src/UI/HUD/ChatHUD.cpp" "src/Input/GameAction.h" "src/Input/ActionState.h" "src/Game/MacroController.h" "src/Game/MacroController.cpp")

if(MSVC)
    target_compile_options(AgarClient PRIVATE /W4 /permissive- /utf-8)
else()
    target_compile_options(AgarClient PRIVATE -Wall -Wextra -Wshadow)
endif()

target_compile_definitions(AgarClient PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)

target_link_libraries(AgarClient PRIVATE
    SDL3::SDL3
    glad::glad
    opengl32
    ixwebsocket::ixwebsocket
    Freetype::Freetype
    bcrypt
    winhttp
)

target_include_directories(AgarClient PRIVATE ${Stb_INCLUDE_DIR})

add_custom_command(TARGET AgarClient POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets
        $<TARGET_FILE_DIR:AgarClient>/assets
    COMMENT "Copying assets"
)