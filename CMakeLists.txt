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

add_executable(AgarClient
    src/main.cpp
    src/Graphics/Window.cpp
    src/Graphics/Shader.cpp
    src/Input/InputManager.cpp
    src/Core/FrameStats.cpp
)

target_link_libraries(AgarClient PRIVATE
    SDL3::SDL3
    glad::glad
    opengl32
)