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

find_package(Stb REQUIRED)

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
    src/Input/InputManager.cpp
    src/Core/FrameStats.cpp
    src/Network/NetworkClient.cpp
    src/Network/PacketHandler.cpp
    src/Game/World.cpp
)

target_link_libraries(AgarClient PRIVATE
    SDL3::SDL3
    glad::glad
    opengl32
    ixwebsocket::ixwebsocket
    bcrypt
    winhttp
)

target_include_directories(AgarClient PRIVATE ${Stb_INCLUDE_DIR})