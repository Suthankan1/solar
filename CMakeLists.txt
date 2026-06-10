cmake_minimum_required(VERSION 3.15)
project(SolarSystem LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include directories
include_directories(
    include
    glad/include
    /opt/homebrew/include
)

# Link directories (Homebrew path for GLFW)
link_directories(
    /opt/homebrew/lib
)

# Target source files
set(SOURCES
    src/main.cpp
    src/core/Window.cpp
    src/core/Renderer.cpp
    src/core/Shader.cpp
    src/core/Mesh.cpp
    src/core/RendererComponent.cpp
    src/scene/SceneManager.cpp
    src/scene/Transform.cpp
    src/scene/Entity.cpp
    src/scene/TransformComponent.cpp
    src/scene/Skybox.cpp
    src/camera/Camera.cpp
    src/celestial/Planet.cpp
    src/celestial/Moon.cpp
    src/celestial/Sun.cpp
    src/celestial/Orbit.cpp
    src/celestial/Starfield.cpp
    src/celestial/SaturnRings.cpp
    include/celestial/SaturnRings.h
    src/celestial/SunHalo.cpp
    include/celestial/SunHalo.h
    src/celestial/AsteroidBelt.cpp
    include/celestial/AsteroidBelt.h
    src/animation/AnimationComponent.cpp
    src/lighting/LightingComponent.cpp
    src/utilities/TextRenderer.cpp
    glad/src/glad.c
)

add_executable(solar ${SOURCES})

# Target compiler warning flags
target_compile_options(solar PRIVATE -Wall -Wextra -Wno-deprecated-declarations)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_options(solar PRIVATE -O2)
endif()

# Frameworks and library links on macOS
if(APPLE)
    target_link_libraries(solar PRIVATE
        glfw
        "-framework OpenGL"
        "-framework Cocoa"
        "-framework IOKit"
        "-framework CoreVideo"
    )
else()
    # Fallback support for non-macOS platforms
    find_package(glfw3 REQUIRED)
    find_package(OpenGL REQUIRED)
    target_link_libraries(solar PRIVATE glfw OpenGL::GL)
endif()
