#include "Window.h"
#include "Renderer.h"
#include <iostream>
#include <exception>
#include <cstdlib>

int main() {
    try {
        std::cout << "Starting Modern OpenGL Application...\n";

        // Create the window wrapper (1280x720 as requested)
        // This initializes GLFW, OpenGL 3.3 Core profile context, and GLAD.
        Window window(1280, 720, "Modern OpenGL 3.3 Project (GLFW/GLAD/GLM)");

        // Create and initialize the renderer
        Renderer renderer;
        if (!renderer.init()) {
            std::cerr << "Fatal Error: Failed to initialize renderer components (shaders/buffers).\n";
            return EXIT_FAILURE;
        }

        std::cout << "Main loop started. Render target dimensions: " 
                  << window.getWidth() << "x" << window.getHeight() << "\n";
        std::cout << "Press [ESC] inside the window to exit the application.\n";

        // Main render loop
        while (!window.shouldClose()) {
            // Get current time in seconds for rotation calculations
            float currentTime = static_cast<float>(glfwGetTime());

            // Draw current frame
            renderer.render(window.getWidth(), window.getHeight(), currentTime);

            // Swap the front and back buffers
            window.swapBuffers();

            // Poll for and process keyboard/window events
            window.pollEvents();
        }

        std::cout << "Exiting application cleanly.\n";

    } catch (const std::exception& e) {
        std::cerr << "Fatal Exception occurred: " << e.what() << "\n";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
