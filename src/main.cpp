#include "core/Window.h"
#include "core/Renderer.h"
#include "scene/SceneManager.h"
#include "celestial/Sun.h"
#include "celestial/SunHalo.h"
#include "celestial/Planet.h"
#include "celestial/Moon.h"
#include "celestial/Orbit.h"
#include "celestial/SaturnRings.h"
#include "celestial/AsteroidBelt.h"
#include "camera/Camera.h"
#include "celestial/Starfield.h"
#include "scene/Skybox.h"
#include "utilities/TextRenderer.h"
#include <iostream>
#include <exception>
#include <cstdlib>
#include <memory>

enum class DemoStage { NONE, TRANSLATION, ROTATION, SCALING, LIGHTING, ZBUFFER, DONE };

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
        auto sunHalo = std::make_shared<SunHalo>("SunHalo", 1.3f, 2.8f);
        // Create Planets (name, radius, orbitRadius, orbitSpeed, rotationSpeed, color)
        auto mercury = std::make_shared<Planet>("Mercury", 0.08f, 2.0f, 1.6f, 3.0f, glm::vec3(0.76f, 0.70f, 0.65f));
        auto venus = std::make_shared<Planet>("Venus", 0.15f, 2.8f, 1.1f, 0.4f, glm::vec3(0.95f, 0.85f, 0.55f));
        auto earth = std::make_shared<Planet>("Earth", 0.18f, 3.8f, 0.9f, 1.8f, glm::vec3(0.25f, 0.55f, 0.95f));
        auto mars = std::make_shared<Planet>("Mars", 0.12f, 5.0f, 0.7f, 1.7f, glm::vec3(0.80f, 0.35f, 0.20f));
        auto asteroidBelt = std::make_shared<AsteroidBelt>("AsteroidBelt", 5.6f, 6.4f, 2000);
        auto jupiter = std::make_shared<Planet>("Jupiter", 0.42f, 7.0f, 0.4f, 2.5f, glm::vec3(0.85f, 0.72f, 0.58f));
        auto saturn = std::make_shared<Planet>("Saturn", 0.36f, 9.5f, 0.3f, 2.2f, glm::vec3(0.92f, 0.86f, 0.65f));
        auto saturnRings = std::make_shared<SaturnRings>("SaturnRings", saturn);
        auto uranus = std::make_shared<Planet>("Uranus", 0.25f, 11.5f, 0.2f, 1.4f, glm::vec3(0.55f, 0.85f, 0.90f));
        auto neptune = std::make_shared<Planet>("Neptune", 0.23f, 13.5f, 0.15f, 1.5f, glm::vec3(0.25f, 0.40f, 0.95f));

        // Create Orbit Rings (name + "Orbit", radius, color, parent)
        auto mercuryOrbit = std::make_shared<Orbit>("MercuryOrbit", mercury->getOrbitRadius(), glm::vec3(0.4f, 0.38f, 0.36f), nullptr);
        auto venusOrbit = std::make_shared<Orbit>("VenusOrbit", venus->getOrbitRadius(), glm::vec3(0.5f, 0.45f, 0.30f), nullptr);
        auto earthOrbit = std::make_shared<Orbit>("EarthOrbit", earth->getOrbitRadius(), glm::vec3(0.15f, 0.30f, 0.55f), nullptr);
        auto marsOrbit = std::make_shared<Orbit>("MarsOrbit", mars->getOrbitRadius(), glm::vec3(0.45f, 0.20f, 0.12f), nullptr);
        auto jupiterOrbit = std::make_shared<Orbit>("JupiterOrbit", jupiter->getOrbitRadius(), glm::vec3(0.45f, 0.38f, 0.30f), nullptr);
        auto saturnOrbit = std::make_shared<Orbit>("SaturnOrbit", saturn->getOrbitRadius(), glm::vec3(0.48f, 0.44f, 0.33f), nullptr);
        auto uranusOrbit = std::make_shared<Orbit>("UranusOrbit", uranus->getOrbitRadius(), glm::vec3(0.28f, 0.42f, 0.46f), nullptr);
        auto neptuneOrbit = std::make_shared<Orbit>("NeptuneOrbit", neptune->getOrbitRadius(), glm::vec3(0.12f, 0.20f, 0.48f), nullptr);

        // Create Moons (name, radius, orbitRadius, orbitSpeed, rotationSpeed, color, parentPlanet)
        auto moon = std::make_shared<Moon>("Moon", 0.06f, 0.38f, 2.5f, 1.2f, glm::vec3(0.75f, 0.75f, 0.72f), earth);
        auto phobos = std::make_shared<Moon>("Phobos", 0.035f, 0.22f, 4.0f, 2.0f, glm::vec3(0.60f, 0.55f, 0.48f), mars);
        auto io = std::make_shared<Moon>("Io", 0.07f, 0.65f, 3.5f, 1.0f, glm::vec3(0.95f, 0.88f, 0.45f), jupiter);
        auto europa = std::make_shared<Moon>("Europa", 0.055f, 0.85f, 2.8f, 1.2f, glm::vec3(0.80f, 0.75f, 0.70f), jupiter);

        // Register everything in order: sun, orbit rings, planets, moons
        sceneManager.registerObject(sunHalo);
        sceneManager.registerObject(sun);

        sceneManager.registerObject(mercuryOrbit);
        sceneManager.registerObject(venusOrbit);
        sceneManager.registerObject(earthOrbit);
        sceneManager.registerObject(marsOrbit);
        sceneManager.registerObject(jupiterOrbit);
        sceneManager.registerObject(saturnOrbit);
        sceneManager.registerObject(uranusOrbit);
        sceneManager.registerObject(neptuneOrbit);

        sceneManager.registerObject(mercury);
        sceneManager.registerObject(venus);
        sceneManager.registerObject(earth);
        sceneManager.registerObject(mars);
        sceneManager.registerObject(asteroidBelt);
        sceneManager.registerObject(jupiter);
        sceneManager.registerObject(saturnRings);  // register BEFORE saturn in the list
        sceneManager.registerObject(saturn);
        sceneManager.registerObject(uranus);
        sceneManager.registerObject(neptune);

        sceneManager.registerObject(moon);
        sceneManager.registerObject(phobos);
        sceneManager.registerObject(io);
        sceneManager.registerObject(europa);
        // 3. Create Skybox Background
        auto starfield = std::make_shared<Starfield>("Starfield", 3000, 80.0f);
        sceneManager.registerObject(starfield);

        auto skybox = std::make_shared<Skybox>("Skybox");
        sceneManager.registerObject(skybox);

        // 4. Create and Register Cameras
        auto sysCamera = std::make_shared<StaticCamera>("SystemCamera", glm::vec3(0.0f, 6.5f, 9.5f), glm::vec3(0.0f, 0.0f, 0.0f));
        auto freeCamera = std::make_shared<FreeCamera>("FreeCamera", glm::vec3(0.0f, 4.0f, 8.0f));

        sceneManager.setActiveCamera(sysCamera);
        window.setCursorMode(GLFW_CURSOR_NORMAL);

        std::cout << "Scene Manager initialized: Objects and cameras registered successfully.\n";
        std::cout << "Press keys [1], [2], [3], or [4] to switch cameras:\n";
        std::cout << "  [1] Global System View (Static Camera)\n";
        std::cout << "  [2] Top-Down View (Static Camera)\n";
        std::cout << "  [3] Side View (Static Camera)\n";
        std::cout << "  [4] Free Fly Camera (WASD to move, Q/E to fly up/down, Mouse to look)\n";
        std::cout << "Press [SPACE] to pause/resume simulation, and [+/-] to speed up/slow down.\n";
        std::cout << "Press [ESC] inside the window to exit.\n";

        double lastTime = glfwGetTime();
        double lastFPSTime = lastTime;
        int frameCount = 0;

        float animationSpeed = 1.0f;   // multiplier
        bool  animationPaused = false;
        bool  spaceWasPressed = false;  // for edge detection
        bool  plusWasPressed = false;
        bool  minusWasPressed = false;

        DemoStage currentDemo = DemoStage::NONE;
        float demoTimer = 0.0f;
        bool f1WasPressed = false;

        auto changeDemoStage = [&](DemoStage nextStage) {
            currentDemo = nextStage;
            demoTimer = 0.0f;

            switch (currentDemo) {
                case DemoStage::NONE:
                    break;
                case DemoStage::TRANSLATION:
                    animationSpeed = 0.5f;
                    animationPaused = false;
                    sceneManager.setActiveCamera(sysCamera);
                    sysCamera->setTargetView(glm::vec3(18.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f));
                    break;
                case DemoStage::ROTATION:
                    animationSpeed = 2.0f;
                    animationPaused = false;
                    sceneManager.setActiveCamera(sysCamera);
                    sysCamera->setTargetView(glm::vec3(0.0f, 2.0f, 4.0f), glm::vec3(0.0f, 0.0f, 0.0f));
                    break;
                case DemoStage::SCALING:
                    animationPaused = true;
                    sceneManager.setActiveCamera(sysCamera);
                    sysCamera->setTargetView(glm::vec3(0.0f, 20.0f, 0.5f), glm::vec3(0.0f, 0.0f, 0.0f));
                    break;
                case DemoStage::LIGHTING:
                    animationPaused = false;
                    animationSpeed = 1.0f;
                    sceneManager.setActiveCamera(sysCamera);
                    sysCamera->setTargetView(glm::vec3(18.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f));
                    break;
                case DemoStage::ZBUFFER:
                    animationPaused = false;
                    animationSpeed = 1.0f;
                    sceneManager.setActiveCamera(sysCamera);
                    sysCamera->setTargetView(glm::vec3(0.0f, 6.5f, 9.5f), glm::vec3(0.0f, 0.0f, 0.0f));
                    break;
                case DemoStage::DONE:
                    animationSpeed = 1.0f;
                    animationPaused = false;
                    sceneManager.setActiveCamera(sysCamera);
                    sysCamera->setTargetView(glm::vec3(0.0f, 6.5f, 9.5f), glm::vec3(0.0f, 0.0f, 0.0f));
                    break;
            }
        };

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
                sysCamera->setTargetView(glm::vec3(0.0f, 6.5f, 9.5f), glm::vec3(0.0f, 0.0f, 0.0f));
            } else if (glfwGetKey(glWin, GLFW_KEY_2) == GLFW_PRESS) {
                sceneManager.setActiveCamera(sysCamera);
                sysCamera->setTargetView(glm::vec3(0.0f, 20.0f, 0.5f), glm::vec3(0.0f, 0.0f, 0.0f));
            } else if (glfwGetKey(glWin, GLFW_KEY_3) == GLFW_PRESS) {
                sceneManager.setActiveCamera(sysCamera);
                sysCamera->setTargetView(glm::vec3(18.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f));
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

            // Input handling for animation controls
            bool spaceIsPressed = (glfwGetKey(glWin, GLFW_KEY_SPACE) == GLFW_PRESS);
            if (spaceIsPressed && !spaceWasPressed) {
                animationPaused = !animationPaused;
            }
            spaceWasPressed = spaceIsPressed;

            bool shiftPressed = (glfwGetKey(glWin, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) ||
                                (glfwGetKey(glWin, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS);
            bool plusIsPressed = ((glfwGetKey(glWin, GLFW_KEY_EQUAL) == GLFW_PRESS) && shiftPressed) ||
                                 (glfwGetKey(glWin, GLFW_KEY_KP_ADD) == GLFW_PRESS);
            if (plusIsPressed && !plusWasPressed) {
                animationSpeed *= 1.5f;
                if (animationSpeed > 8.0f) animationSpeed = 8.0f;
            }
            plusWasPressed = plusIsPressed;

            bool minusIsPressed = (glfwGetKey(glWin, GLFW_KEY_MINUS) == GLFW_PRESS) ||
                                  (glfwGetKey(glWin, GLFW_KEY_KP_SUBTRACT) == GLFW_PRESS);
            if (minusIsPressed && !minusWasPressed) {
                animationSpeed /= 1.5f;
                if (animationSpeed < 0.1f) animationSpeed = 0.1f;
            }
            minusWasPressed = minusIsPressed;

            // Demo mode F1 key detection
            bool f1IsPressed = (glfwGetKey(glWin, GLFW_KEY_F1) == GLFW_PRESS);
            if (f1IsPressed && !f1WasPressed) {
                DemoStage nextStage = DemoStage::NONE;
                switch (currentDemo) {
                    case DemoStage::NONE:        nextStage = DemoStage::TRANSLATION; break;
                    case DemoStage::TRANSLATION: nextStage = DemoStage::ROTATION; break;
                    case DemoStage::ROTATION:    nextStage = DemoStage::SCALING; break;
                    case DemoStage::SCALING:     nextStage = DemoStage::LIGHTING; break;
                    case DemoStage::LIGHTING:    nextStage = DemoStage::ZBUFFER; break;
                    case DemoStage::ZBUFFER:     nextStage = DemoStage::DONE; break;
                    case DemoStage::DONE:        nextStage = DemoStage::NONE; break;
                }
                changeDemoStage(nextStage);
            }
            f1WasPressed = f1IsPressed;

            // Timer update & auto-advance
            if (currentDemo != DemoStage::NONE) {
                demoTimer += deltaTime;
                if (demoTimer >= 8.0f) {
                    DemoStage nextStage = DemoStage::NONE;
                    switch (currentDemo) {
                        case DemoStage::TRANSLATION: nextStage = DemoStage::ROTATION; break;
                        case DemoStage::ROTATION:    nextStage = DemoStage::SCALING; break;
                        case DemoStage::SCALING:     nextStage = DemoStage::LIGHTING; break;
                        case DemoStage::LIGHTING:    nextStage = DemoStage::ZBUFFER; break;
                        case DemoStage::ZBUFFER:     nextStage = DemoStage::DONE; break;
                        case DemoStage::DONE:        nextStage = DemoStage::NONE; break;
                        default:                     nextStage = DemoStage::NONE; break;
                    }
                    changeDemoStage(nextStage);
                }
            }

            // Clear screen
            renderer.clear(glm::vec4(0.04f, 0.04f, 0.06f, 1.0f));

            // Update active camera with real deltaTime
            if (sceneManager.getActiveCamera()) {
                sceneManager.getActiveCamera()->update(deltaTime);
            }

            // Update scene objects
            float effectiveDelta = animationPaused ? 0.0f : (deltaTime * animationSpeed);
            sceneManager.update(effectiveDelta);

            // Render scene
            sceneManager.render(renderer);

            // Render text overlays (instructions in corners)
            int width = window.getWidth();
            int height = window.getHeight();
            
            // TOP-LEFT block — Solar System title:
            textRenderer.renderText("SOLAR SYSTEM SIMULATION", 15.0f, height - 25.0f, 2.0f, glm::vec3(1.0f, 0.85f, 0.3f), width, height);
            textRenderer.renderText("OpenGL | Computer Graphics", 15.0f, height - 50.0f, 1.2f, glm::vec3(0.8f, 0.8f, 0.8f), width, height);

            // CENTER-TOP — Demo Stage Showcase
            std::string demoText = "";
            if (currentDemo == DemoStage::TRANSLATION) {
                demoText = "Demonstrating: Translation";
            } else if (currentDemo == DemoStage::ROTATION) {
                demoText = "Demonstrating: Rotation (Self-Axis)";
            } else if (currentDemo == DemoStage::SCALING) {
                demoText = "Demonstrating: Scaling (Planet Sizes)";
            } else if (currentDemo == DemoStage::LIGHTING) {
                demoText = "Demonstrating: Phong Lighting (Ambient + Diffuse + Specular)";
            } else if (currentDemo == DemoStage::ZBUFFER) {
                demoText = "Demonstrating: Z-Buffer / Depth Testing";
            } else if (currentDemo == DemoStage::DONE) {
                demoText = "Demo Complete - Resetting";
            }

            if (!demoText.empty()) {
                float demoScale = 1.8f;
                float textWidth = demoText.length() * 8.0f * demoScale;
                float xPos = (width - textWidth) / 2.0f;
                float yPos = height - 55.0f;
                textRenderer.renderText(demoText, xPos, yPos, demoScale, glm::vec3(0.2f, 1.0f, 0.6f), width, height);
            }

            // BOTTOM-LEFT block — Controls:
            textRenderer.renderText("[1] Global  [2] Top  [3] Side  [4] Free", 15.0f, 75.0f, 1.1f, glm::vec3(1.0f, 1.0f, 1.0f), width, height);
            textRenderer.renderText("[SPACE] Pause  [+/-] Speed", 15.0f, 55.0f, 1.1f, glm::vec3(1.0f, 1.0f, 1.0f), width, height);
            textRenderer.renderText("[F1] Demo Mode", 15.0f, 35.0f, 1.1f, glm::vec3(0.2f, 1.0f, 0.6f), width, height);
            textRenderer.renderText("[ESC] Exit", 15.0f, 15.0f, 1.1f, glm::vec3(0.5f, 0.5f, 0.5f), width, height);

            // BOTTOM-CENTER — PAUSED indicator (only when paused):
            if (animationPaused) {
                std::string pauseText = "⏸ PAUSED";
                float pauseScale = 2.0f;
                float offset = (pauseText.length() * 8.0f * pauseScale) / 2.0f;
                textRenderer.renderText(pauseText, (width / 2.0f) - offset, 35.0f, pauseScale, glm::vec3(1.0f, 0.5f, 0.0f), width, height);
            }

            // TOP-RIGHT — Speed and camera info:
            std::string camName = sceneManager.getActiveCamera() ? sceneManager.getActiveCamera()->getName() : "None";
            std::string camStr = "Camera: " + camName;
            float camScale = 1.3f;
            float camX = width - (camStr.length() * 8.0f * camScale) - 15.0f;
            textRenderer.renderText(camStr, camX, height - 25.0f, camScale, glm::vec3(0.0f, 1.0f, 1.0f), width, height);

            std::string speedOverlayStr;
            glm::vec3 speedColor(1.0f, 1.0f, 1.0f);
            if (animationPaused) {
                speedOverlayStr = "Speed: PAUSED";
            } else {
                char speedBufVal[32];
                std::snprintf(speedBufVal, sizeof(speedBufVal), "%.1f", animationSpeed);
                speedOverlayStr = "Speed: " + std::string(speedBufVal) + "x";
                if (animationSpeed > 1.5f) {
                    speedColor = glm::vec3(1.0f, 1.0f, 0.0f);
                }
            }
            float speedScale = 1.3f;
            float speedX = width - (speedOverlayStr.length() * 8.0f * speedScale) - 15.0f;
            textRenderer.renderText(speedOverlayStr, speedX, height - 50.0f, speedScale, speedColor, width, height);

            // Swap buffers & poll events
            window.swapBuffers();
            window.pollEvents();

            // Calculate FPS & update window title
            frameCount++;
            if (currentTime - lastFPSTime >= 1.0) {
                double fps = frameCount / (currentTime - lastFPSTime);
                std::string fps_str = std::to_string(static_cast<int>(fps));
                
                char speedBufVal[32];
                std::snprintf(speedBufVal, sizeof(speedBufVal), "%.1f", animationSpeed);
                std::string speed_str = speedBufVal;
                
                std::string camName = sceneManager.getActiveCamera() ? sceneManager.getActiveCamera()->getName() : "None";
                
                std::string demoSuffix = "";
                if (currentDemo != DemoStage::NONE) {
                    if (currentDemo == DemoStage::TRANSLATION) demoSuffix = " | Demo: Translation";
                    else if (currentDemo == DemoStage::ROTATION) demoSuffix = " | Demo: Rotation";
                    else if (currentDemo == DemoStage::SCALING) demoSuffix = " | Demo: Scaling";
                    else if (currentDemo == DemoStage::LIGHTING) demoSuffix = " | Demo: Lighting";
                    else if (currentDemo == DemoStage::ZBUFFER) demoSuffix = " | Demo: Z-Buffer";
                    else if (currentDemo == DemoStage::DONE) demoSuffix = " | Demo: Done";
                }

                std::string title = "Solar System | FPS: " + fps_str + " | Speed: " + speed_str + "x | " + camName + demoSuffix;
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
