#include "core/Window.h"
#include "core/Renderer.h"
#include "scene/SceneManager.h"
#include "celestial/Sun.h"
#include "celestial/Planet.h"
#include "celestial/Moon.h"
#include "celestial/Orbit.h"
#include "camera/Camera.h"
#include "celestial/Starfield.h"
#include "scene/Skybox.h"
#include "utilities/TextRenderer.h"
#include <iostream>
#include <exception>
#include <cstdlib>
#include <memory>

int main() {
    try {
        std::cout << "Starting Modern OpenGL Solar System Application...\n";

        // Seed random number generator
        std::srand(12345);

        // Create the window wrapper (1280x720 as requested)
        Window window(1280, 720, "Solar System");

        // Create and initialize the renderer
        Renderer renderer;
        if (!renderer.init()) {
            std::cerr << "Fatal Error: Failed to initialize renderer components (shaders/buffers).\n";
            return EXIT_FAILURE;
        }

        // Initialize Scene Manager
        SceneManager sceneManager;

        // Initialize Text Renderer
        TextRenderer textRenderer;
        if (!textRenderer.init()) {
            std::cerr << "Warning: Failed to initialize text renderer overlay.\n";
        }

        // 1. Create Celestial Objects (Stars, Planets, Moons)
        // Central Sun
        auto sun = std::make_shared<Sun>("Sun", 1.2f, glm::vec3(1.0f, 0.65f, 0.0f), glm::vec3(1.0f, 0.95f, 0.8f));
        sceneManager.registerObject(sun);


        // 3. Create Skybox Background
        auto skybox = std::make_shared<Skybox>("Skybox");
        sceneManager.registerObject(skybox);

        // 4. Create and Register Cameras
        auto sysCamera = std::make_shared<StaticCamera>("SystemCamera", glm::vec3(0.0f, 6.5f, 9.5f), glm::vec3(0.0f, 0.0f, 0.0f));
        auto freeCamera = std::make_shared<FreeCamera>("FreeCamera", glm::vec3(0.0f, 4.0f, 8.0f));

        sceneManager.setActiveCamera(sysCamera);
        window.setCursorMode(GLFW_CURSOR_NORMAL);

        std::cout << "Scene Manager initialized: Objects and cameras registered successfully.\n";
        std::cout << "Press key [1] or [4] to switch cameras:\n";
        std::cout << "  [1] Global System View (Static Camera)\n";
        std::cout << "  [4] Free Fly Camera (WASD to move, Q/E to fly up/down, Arrow keys to look)\n";
        std::cout << "Press [ESC] inside the window to exit.\n";

        double lastTime = glfwGetTime();
        double lastFPSTime = lastTime;
        int frameCount = 0;

        // Main render loop
        while (!window.shouldClose()) {
            double currentTime = glfwGetTime();
            float deltaTime = static_cast<float>(currentTime - lastTime);
            lastTime = currentTime;

            // Process camera switching input
            GLFWwindow* glWin = window.getGLFWWindow();
            std::shared_ptr<Camera> oldCam = sceneManager.getActiveCamera();
            if (glfwGetKey(glWin, GLFW_KEY_1) == GLFW_PRESS) {
                sceneManager.setActiveCamera(sysCamera);
            } else if (glfwGetKey(glWin, GLFW_KEY_4) == GLFW_PRESS) {
                sceneManager.setActiveCamera(freeCamera);
            }

            if (sceneManager.getActiveCamera() != oldCam) {
                if (sceneManager.getActiveCamera() == freeCamera) {
                    window.setCursorMode(GLFW_CURSOR_DISABLED);
                } else {
                    window.setCursorMode(GLFW_CURSOR_NORMAL);
                }
            }

            // Clear screen
            renderer.clear(glm::vec4(0.04f, 0.04f, 0.06f, 1.0f));

            // Update scene objects
            sceneManager.update(deltaTime);

            // Render scene
            sceneManager.render(renderer);

            // Render text overlays (instructions in corners)
            int width = window.getWidth();
            int height = window.getHeight();
            
            // Left Corner Instructions: Camera Status & Key Selection
            textRenderer.renderText("CAMERA SYSTEM STATUS", 15.0f, height - 25.0f, 1.8f, glm::vec3(1.0f, 0.75f, 0.0f), width, height);
            
            std::string activeCamStr = "Active: " + (sceneManager.getActiveCamera() ? sceneManager.getActiveCamera()->getName() : "None");
            textRenderer.renderText(activeCamStr, 15.0f, height - 45.0f, 1.4f, glm::vec3(1.0f, 1.0f, 1.0f), width, height);
            
            textRenderer.renderText("Switches: [1] Global View  [4] Free Look", 15.0f, height - 65.0f, 1.2f, glm::vec3(0.7f, 0.8f, 0.9f), width, height);

            // Right Corner Instructions: Controls specific to camera
            if (sceneManager.getActiveCamera() == freeCamera) {
                std::string header = "FREE LOOK CONTROLS";
                std::string line1  = "Move: W A S D  |  Fly: Q E";
                std::string line2  = "Look Around: Mouse Movement";
                std::string line3  = "Zoom: Mouse Scroll Wheel";
                
                float xHeader = width - (header.length() * 8.0f * 1.6f) - 15.0f;
                float xLine1  = width - (line1.length() * 8.0f * 1.2f) - 15.0f;
                float xLine2  = width - (line2.length() * 8.0f * 1.2f) - 15.0f;
                float xLine3  = width - (line3.length() * 8.0f * 1.2f) - 15.0f;
                
                textRenderer.renderText(header, xHeader, height - 25.0f, 1.6f, glm::vec3(0.0f, 0.8f, 1.0f), width, height);
                textRenderer.renderText(line1,  xLine1,  height - 45.0f, 1.2f, glm::vec3(0.9f, 0.9f, 0.9f), width, height);
                textRenderer.renderText(line2,  xLine2,  height - 65.0f, 1.2f, glm::vec3(0.9f, 0.9f, 0.9f), width, height);
                textRenderer.renderText(line3,  xLine3,  height - 85.0f, 1.2f, glm::vec3(0.9f, 0.9f, 0.9f), width, height);
            } else {
                std::string header = "STATIC VIEWMODE";
                std::string line1  = "Use Key [4] to enter Free Look";
                std::string line2  = "to unlock mouse/zoom control";
                
                float xHeader = width - (header.length() * 8.0f * 1.6f) - 15.0f;
                float xLine1  = width - (line1.length() * 8.0f * 1.2f) - 15.0f;
                float xLine2  = width - (line2.length() * 8.0f * 1.2f) - 15.0f;
                
                textRenderer.renderText(header, xHeader, height - 25.0f, 1.6f, glm::vec3(0.8f, 0.8f, 0.8f), width, height);
                textRenderer.renderText(line1,  xLine1,  height - 45.0f, 1.2f, glm::vec3(0.6f, 0.6f, 0.6f), width, height);
                textRenderer.renderText(line2,  xLine2,  height - 65.0f, 1.2f, glm::vec3(0.6f, 0.6f, 0.6f), width, height);
            }

            // Swap buffers & poll events
            window.swapBuffers();
            window.pollEvents();

            // Calculate FPS & update window title
            frameCount++;
            if (currentTime - lastFPSTime >= 1.0) {
                double fps = frameCount / (currentTime - lastFPSTime);
                std::string camName = sceneManager.getActiveCamera() ? sceneManager.getActiveCamera()->getName() : "None";
                std::string title = "Solar System | FPS: " + std::to_string(static_cast<int>(fps)) + " | Active Camera: " + camName;
                window.setTitle(title);

                frameCount = 0;
                lastFPSTime = currentTime;
            }
        }

        std::cout << "Exiting application cleanly.\n";
        sceneManager.clear();

    } catch (const std::exception& e) {
        std::cerr << "Fatal Exception occurred: " << e.what() << "\n";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
