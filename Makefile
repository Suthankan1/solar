# Makefile for Modern OpenGL 3.3 Project on macOS
# Using GLFW, GLAD, and GLM

CXX = clang++
CC = clang
CXXFLAGS = -Wall -Wextra -std=c++17 -g -Wno-deprecated-declarations -Iinclude -Iglad/include -I/opt/homebrew/include
CFLAGS = -Wall -Wextra -g -Iglad/include
LDFLAGS = -L/opt/homebrew/lib -lglfw -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo

TARGET = solar

CPPSRC = src/main.cpp \
         src/core/Window.cpp \
         src/core/Renderer.cpp \
         src/core/Shader.cpp \
         src/core/Mesh.cpp \
         src/core/Texture.cpp \
         src/core/RendererComponent.cpp \
         src/scene/SceneManager.cpp \
         src/scene/Transform.cpp \
         src/scene/Entity.cpp \
         src/scene/TransformComponent.cpp \
         src/scene/Skybox.cpp \
         src/camera/Camera.cpp \
         src/celestial/Planet.cpp \
         src/celestial/Moon.cpp \
         src/celestial/SpaceStation.cpp \
         src/celestial/Sun.cpp \
         src/celestial/Orbit.cpp \
         src/celestial/Starfield.cpp \
         src/celestial/SaturnRings.cpp \
         src/celestial/SunHalo.cpp \
         src/celestial/AsteroidBelt.cpp \
         src/celestial/CloudLayer.cpp \
         src/celestial/AtmosphereLayer.cpp \
         src/celestial/Spacecraft.cpp \
         src/animation/AnimationComponent.cpp \
         src/lighting/LightingComponent.cpp \
         src/utilities/TextRenderer.cpp

CSRC = glad/src/glad.c

CPPOBJ = $(CPPSRC:.cpp=.o)
COBJ = $(CSRC:.c=.o)

all: $(TARGET)

$(TARGET): $(CPPOBJ) $(COBJ)
	$(CXX) $(CPPOBJ) $(COBJ) -o $(TARGET) $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) $(CPPOBJ) $(COBJ)
	rm -rf $(TARGET).dSYM

.PHONY: all clean
