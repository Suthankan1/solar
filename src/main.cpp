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
#include "celestial/CloudLayer.h"
#include "celestial/AtmosphereLayer.h"
#include "celestial/SpaceStation.h"
#include "celestial/Spacecraft.h"
#include "camera/Camera.h"
#include "celestial/Starfield.h"
#include "scene/Skybox.h"
#include "utilities/TextRenderer.h"
#include <iostream>
#include <exception>
#include <cstdlib>
#include <memory>
#include <vector>
#include <sstream>

struct PlanetInfo {
    std::string name;
    std::string type;
    std::string features;
    std::string moons;
    std::string concepts;
};

static PlanetInfo getPlanetInfo(const std::string& name) {
    if (name == "Mercury") {
        return { "MERCURY", "Terrestrial planet", "Scorched rocky surface, close orbit", "0", "Rotation, revolution, lighting, scaling" };
    } else if (name == "Venus") {
        return { "VENUS", "Terrestrial planet", "Dense atmosphere, high reflection", "0", "Rotation, revolution, lighting" };
    } else if (name == "Earth") {
        return { "EARTH", "Terrestrial planet", "Texture mapping, atmosphere, cloud layer", "1 (Moon)", "Rotation, revolution, lighting, blending" };
    } else if (name == "Mars") {
        return { "MARS", "Terrestrial planet", "Red surface texture, Phobos orbit", "1 (Phobos)", "Rotation, revolution, lighting" };
    } else if (name == "Jupiter") {
        return { "JUPITER", "Gas giant", "Banded gas surface, large scale", "2 (Io, Europa)", "Rotation, revolution, lighting, scaling" };
    } else if (name == "Saturn") {
        return { "SATURN", "Gas giant", "Spectacular rings, custom ring mesh", "0 (registered)", "Rotation, revolution, lighting, alpha blending" };
    } else if (name == "Uranus") {
        return { "URANUS", "Ice giant", "Cyan atmospheric tint, tilted axis", "0", "Rotation, revolution, lighting, axis tilt" };
    } else if (name == "Neptune") {
        return { "NEPTUNE", "Ice giant", "Deep blue methane atmosphere, remote orbit", "0", "Rotation, revolution, lighting, orbit tracing" };
    } else if (name == "Sun") {
        return { "SUN", "Yellow dwarf star", "Procedural texture, corona halo glow", "0", "Self-rotation, point light source, alpha blending" };
    } else if (name == "SpaceStation") {
        return { "SPACE STATION", "ISS-style orbital habitat", "Truss cylinders, solar panels", "0", "Relative orbit, compound meshes, hierarchy" };
    } else if (name == "Spacecraft") {
        return { "SPACECRAFT", "Interplanetary explorer", "Custom probe mesh, trajectory path", "0", "Hohmann transfer, follow camera, lerp interpolation" };
    } else if (name == "AsteroidBelt") {
        return { "ASTEROID BELT", "Debris field", "2000 point-size orbiting particles", "0", "GL_POINTS, instanced rendering, particle simulation" };
    }
    return { name, "Celestial Body", "Astronomical object orbiting in space", "0", "Rotation, revolution, lighting" };
}

enum class DemoStage {
    NONE,
    OPENING_WIDE,
    SUN_ZOOM,
    INNER_PLANETS,
    EARTH_MOON,
    SPACE_STATION,
    SPACECRAFT,
    ASTEROID_BELT,
    JUPITER_SATURN,
    CREDITS
};

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
        auto sun = std::make_shared<Sun>("Sun", 1.2f, glm::vec3(1.0f, 0.65f, 0.0f), glm::vec3(1.0f, 0.95f, 0.8f), "textures/sun.jpg");
        auto sunHalo = std::make_shared<SunHalo>("SunHalo", 1.3f, 2.8f);
        // Create Planets (name, radius, orbitRadius, orbitSpeed, rotationSpeed, color)
        auto mercury = std::make_shared<Planet>("Mercury", 0.08f, 2.0f, 1.6f, 3.0f, glm::vec3(0.76f, 0.70f, 0.65f), "textures/mercury.jpg");
        auto venus = std::make_shared<Planet>("Venus", 0.15f, 2.8f, 1.1f, 0.4f, glm::vec3(0.95f, 0.85f, 0.55f), "textures/venus.jpg");
        auto earth = std::make_shared<Planet>("Earth", 0.18f, 3.8f, 0.9f, 1.8f, glm::vec3(0.25f, 0.55f, 0.95f), "textures/earth.jpg");
        auto mars = std::make_shared<Planet>("Mars", 0.12f, 5.0f, 0.7f, 1.7f, glm::vec3(0.80f, 0.35f, 0.20f), "textures/mars.jpg");
        auto asteroidBelt = std::make_shared<AsteroidBelt>("AsteroidBelt", 5.6f, 6.4f, 2000);
        auto jupiter = std::make_shared<Planet>("Jupiter", 0.42f, 7.0f, 0.4f, 2.5f, glm::vec3(0.85f, 0.72f, 0.58f), "textures/jupiter.jpg");
        auto saturn = std::make_shared<Planet>("Saturn", 0.36f, 9.5f, 0.3f, 2.2f, glm::vec3(0.92f, 0.86f, 0.65f), "textures/saturn.jpg");
        auto saturnRings = std::make_shared<SaturnRings>("SaturnRings", saturn);
        auto uranus = std::make_shared<Planet>("Uranus", 0.25f, 11.5f, 0.2f, 1.4f, glm::vec3(0.55f, 0.85f, 0.90f), "textures/uranus.jpg");
        auto neptune = std::make_shared<Planet>("Neptune", 0.23f, 13.5f, 0.15f, 1.5f, glm::vec3(0.25f, 0.40f, 0.95f), "textures/neptune.jpg");

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
        auto moon = std::make_shared<Moon>("Moon", 0.06f, 0.38f, 2.5f, 1.2f, glm::vec3(0.75f, 0.75f, 0.72f), earth, "textures/moon.jpg");
        auto phobos = std::make_shared<Moon>("Phobos", 0.035f, 0.22f, 4.0f, 2.0f, glm::vec3(0.60f, 0.55f, 0.48f), mars);
        auto io = std::make_shared<Moon>("Io", 0.07f, 0.65f, 3.5f, 1.0f, glm::vec3(0.95f, 0.88f, 0.45f), jupiter);
        auto europa = std::make_shared<Moon>("Europa", 0.055f, 0.85f, 2.8f, 1.2f, glm::vec3(0.80f, 0.75f, 0.70f), jupiter);

        // Create Space Station and its Orbit orbiting Earth
        auto spaceStation = std::make_shared<SpaceStation>("SpaceStation", 0.28f, 2.2f, earth);
        auto spaceStationOrbit = std::make_shared<Orbit>("SpaceStationOrbit", 0.28f, glm::vec3(0.2f, 0.35f, 0.45f), earth);

        // Create Spacecraft
        auto spacecraft = std::make_shared<Spacecraft>("Spacecraft", earth, mars);

        // Register everything in order: sun, orbit rings, planets, moons
        sceneManager.registerObject(sunHalo);
        sceneManager.registerObject(sun);

        sceneManager.registerObject(mercuryOrbit);
        sceneManager.registerObject(venusOrbit);
        sceneManager.registerObject(earthOrbit);
        sceneManager.registerObject(spaceStationOrbit);
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
        sceneManager.registerObject(spaceStation);
        sceneManager.registerObject(spacecraft);
        sceneManager.registerObject(phobos);
        sceneManager.registerObject(io);
        sceneManager.registerObject(europa);

        // Instantiate and register Earth's transparent layers (clouds & atmosphere)
        auto earthClouds = std::make_shared<CloudLayer>("EarthClouds", earth, 1.015f, 1.95f);
        auto earthAtmosphere = std::make_shared<AtmosphereLayer>("EarthAtmosphere", earth, 1.035f, glm::vec3(0.25f, 0.55f, 1.0f));
        sceneManager.registerObject(earthClouds);
        sceneManager.registerObject(earthAtmosphere);

        // 3. Create Skybox Background
        auto starfield = std::make_shared<Starfield>("Starfield", 3000, 80.0f);
        sceneManager.registerObject(starfield);

        auto skybox = std::make_shared<Skybox>("Skybox");
        sceneManager.registerObject(skybox);

        // 4. Create and Register Cameras
        auto sysCamera = std::make_shared<StaticCamera>("SystemCamera", glm::vec3(0.0f, 6.5f, 9.5f), glm::vec3(0.0f, 0.0f, 0.0f));
        auto freeCamera = std::make_shared<FreeCamera>("FreeCamera", glm::vec3(0.0f, 4.0f, 8.0f));
        auto spacecraftCamera = std::make_shared<SpacecraftFollowCamera>("Spacecraft Follow", spacecraft, 0.08f, 0.025f);
        auto trackingCamera = std::make_shared<TrackingCamera>("Planet Focus", earth, glm::vec3(0.0f, 0.36f, 0.72f));

        sceneManager.setActiveCamera(sysCamera);
        sceneManager.registerObject(spacecraftCamera);
        sceneManager.registerObject(trackingCamera);
        window.setCursorMode(GLFW_CURSOR_NORMAL);

        // Vector of all planets for keyboard selection cycling
        std::vector<std::shared_ptr<Planet>> planets = { mercury, venus, earth, mars, jupiter, saturn, uranus, neptune };

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

        // Selection variables
        int selectedPlanetIdx = 2; // Default to Earth
        bool nWasPressed = false;
        bool bWasPressed = false;
        bool enterWasPressed = false;

        DemoStage currentDemo = DemoStage::NONE;
        float demoTimer = 0.0f;
        bool f1WasPressed = false;
        bool demoPaused = false;

        std::shared_ptr<Camera> cameraBeforeDemo = nullptr;
        int cursorModeBeforeDemo = GLFW_CURSOR_NORMAL;
        float speedBeforeDemo = 1.0f;
        glm::vec3 demoStartCamPos(0.0f);
        glm::vec3 demoStartCamFront(0.0f, 0.0f, -1.0f);
        int currentSubStageIdx = 0;

        struct DemoSubStage {
            DemoStage stage;
            std::string name;
            float startTime;
            float duration;
            std::string title;
            std::string explanation;
            std::string highlight;
            float speed;
        };

        std::vector<DemoSubStage> subStages = {
            { DemoStage::OPENING_WIDE,   "Opening",                     0.0f,   15.0f, "Realistic Solar System Explorer", "An interactive OpenGL simulation of our celestial neighborhood, showcasing orbits, textures, lighting, and spacecraft.", "",             1.0f },
            { DemoStage::SUN_ZOOM,       "Sun Zoom",                   15.0f,   20.0f, "Sun: Dynamic light source",        "The central star. Animates with a procedural halo glow and serves as the primary point light source for the system.", "Sun",        1.0f },
            { DemoStage::INNER_PLANETS,  "Mercury View",               35.0f,    6.0f, "Inner Planets: Mercury",           "The smallest and closest planet to the Sun, orbiting at high speed in a scorched environment.", "Mercury",    2.5f },
            { DemoStage::INNER_PLANETS,  "Venus View",                 41.0f,    6.0f, "Inner Planets: Venus",             "Earth's sister planet, shrouded in a dense, reflective atmosphere with extreme greenhouse temperatures.", "Venus",      2.2f },
            { DemoStage::INNER_PLANETS,  "Earth View",                 47.0f,    6.0f, "Inner Planets: Earth",             "Our home planet. The only known world hosting liquid surface water and life.", "Earth",      2.0f },
            { DemoStage::INNER_PLANETS,  "Mars View",                  53.0f,    7.0f, "Inner Planets: Mars",              "The Red Planet, featuring iron-rich soil, polar ice caps, and the solar system's tallest volcano.", "Mars",       2.0f },
            { DemoStage::EARTH_MOON,     "Earth & Moon",               60.0f,   20.0f, "Earth, Moon & Atmosphere",         "Rendered with a multi-layered atmospheric scattering glow and a separate, slowly rotating, semi-transparent cloud layer.", "Earth",     1.0f },
            { DemoStage::SPACE_STATION,  "Space Station Orbit",        80.0f,   20.0f, "International Space Station",      "A human-made modular spacecraft orbiting Earth, assembled from cylinders, solar panels, and metallic trusses.", "SpaceStation", 2.2f },
            { DemoStage::SPACECRAFT,     "Spacecraft Mission",        100.0f,   25.0f, "Interplanetary Spacecraft Mission","Following an astronaut spacecraft executing a Hohmann transfer trajectory from Earth to Mars.", "Spacecraft", 1.5f },
            { DemoStage::ASTEROID_BELT,  "Asteroid Belt",             125.0f,   20.0f, "Asteroid Belt Fly-through",        "An annular region between Mars and Jupiter filled with thousands of rocky bodies orbiting the Sun.", "AsteroidBelt", 1.2f },
            { DemoStage::JUPITER_SATURN, "Jupiter View",              145.0f,   10.0f, "Outer Planets: Jupiter",           "The largest planet in our solar system, a massive gas giant with famous banding and numerous moons.", "Jupiter",    1.0f },
            { DemoStage::JUPITER_SATURN, "Saturn Rings View",         155.0f,   10.0f, "Outer Planets: Saturn & Rings",     "Featuring a spectacular, tilted ring system composed of billions of ice particles, rocky debris, and dust.", "Saturn",     1.0f },
            { DemoStage::CREDITS,        "Final Wide View",           165.0f,   15.0f, "Solar System Explorer Credits",    "Built using C++, Modern OpenGL, GLFW, Glad, and GLM. Thank you for watching!", "",             1.0f }
        };

        auto getSubStageIdealState = [&](int idx, float t, glm::vec3& pos, glm::vec3& lookAt) {
            switch (idx) {
                case 0: { // Wide shot (0-15s)
                    pos = glm::vec3(std::cos(t * 0.12f) * 18.0f, 7.5f + std::sin(t * 0.08f) * 2.5f, std::sin(t * 0.12f) * 18.0f);
                    lookAt = glm::vec3(0.0f, 0.0f, 0.0f);
                    break;
                }
                case 1: { // Sun close-up (15-35s)
                    pos = glm::vec3(std::cos(t * 0.3f) * 3.2f, 0.8f, std::sin(t * 0.3f) * 3.2f);
                    lookAt = glm::vec3(0.0f, 0.0f, 0.0f);
                    break;
                }
                case 2: { // Mercury View (35-41s)
                    glm::vec3 mercuryPos = mercury ? mercury->getPosition() : glm::vec3(0.0f);
                    pos = mercuryPos + glm::vec3(std::cos(t * 1.2f) * 0.28f, 0.10f, std::sin(t * 1.2f) * 0.28f);
                    lookAt = mercuryPos;
                    break;
                }
                case 3: { // Venus View (41-47s)
                    glm::vec3 venusPos = venus ? venus->getPosition() : glm::vec3(0.0f);
                    pos = venusPos + glm::vec3(std::cos(t * 1.0f) * 0.42f, 0.15f, std::sin(t * 1.0f) * 0.42f);
                    lookAt = venusPos;
                    break;
                }
                case 4: { // Earth View (47-53s)
                    glm::vec3 earthPos = earth ? earth->getPosition() : glm::vec3(0.0f);
                    pos = earthPos + glm::vec3(std::cos(t * 0.8f) * 0.50f, 0.20f, std::sin(t * 0.8f) * 0.50f);
                    lookAt = earthPos;
                    break;
                }
                case 5: { // Mars View (53-60s)
                    glm::vec3 marsPos = mars ? mars->getPosition() : glm::vec3(0.0f);
                    pos = marsPos + glm::vec3(std::cos(t * 0.9f) * 0.38f, 0.15f, std::sin(t * 0.9f) * 0.38f);
                    lookAt = marsPos;
                    break;
                }
                case 6: { // Earth Atmosphere & Clouds (60-80s)
                    glm::vec3 earthPos = earth ? earth->getPosition() : glm::vec3(0.0f);
                    pos = earthPos + glm::vec3(std::cos(t * 0.5f) * 0.75f, 0.22f, std::sin(t * 0.5f) * 0.75f);
                    lookAt = earthPos;
                    break;
                }
                case 7: { // Space Station Orbit (80-100s)
                    glm::vec3 stationPos = spaceStation ? spaceStation->getPosition() : glm::vec3(0.0f);
                    pos = stationPos + glm::vec3(std::cos(t * 1.5f) * 0.08f, 0.025f, std::sin(t * 1.5f) * 0.08f);
                    lookAt = stationPos;
                    break;
                }
                case 8: { // Spacecraft Mission (100-125s)
                    glm::vec3 craftPos = spacecraft ? spacecraft->getPosition() : glm::vec3(0.0f);
                    glm::vec3 craftFwd = spacecraft ? spacecraft->getForwardDir() : glm::vec3(0.0f, 0.0f, -1.0f);
                    pos = craftPos + glm::vec3(std::cos(t * 0.6f) * 0.12f, 0.04f, std::sin(t * 0.6f) * 0.12f);
                    lookAt = craftPos + craftFwd * 0.01f;
                    break;
                }
                case 9: { // Asteroid Belt (125-145s)
                    pos = glm::vec3(std::cos(t * 0.08f) * 6.0f, 0.25f, std::sin(t * 0.08f) * 6.0f);
                    lookAt = glm::vec3(std::cos(t * 0.08f + 0.2f) * 6.0f, 0.0f, std::sin(t * 0.08f + 0.2f) * 6.0f);
                    break;
                }
                case 10: { // Jupiter View (145-155s)
                    glm::vec3 jupiterPos = jupiter ? jupiter->getPosition() : glm::vec3(0.0f);
                    pos = jupiterPos + glm::vec3(std::cos(t * 0.4f) * 1.3f, 0.4f, std::sin(t * 0.4f) * 1.3f);
                    lookAt = jupiterPos;
                    break;
                }
                case 11: { // Saturn Rings View (155-165s)
                    glm::vec3 saturnPos = saturn ? saturn->getPosition() : glm::vec3(0.0f);
                    pos = saturnPos + glm::vec3(std::cos(t * 0.35f) * 1.4f, 0.55f, std::sin(t * 0.35f) * 1.4f);
                    lookAt = saturnPos;
                    break;
                }
                case 12: { // Credits/Final Wide (165-180s)
                    pos = glm::vec3(std::cos(t * 0.08f) * 22.0f, 10.0f, std::sin(t * 0.08f) * 22.0f);
                    lookAt = glm::vec3(0.0f, 0.0f, 0.0f);
                    break;
                }
                default: {
                    pos = glm::vec3(0.0f, 10.0f, 15.0f);
                    lookAt = glm::vec3(0.0f);
                    break;
                }
            }
        };

        auto exitDemoMode = [&]() {
            currentDemo = DemoStage::NONE;
            demoTimer = 0.0f;
            demoPaused = false;
            animationPaused = false;
            animationSpeed = speedBeforeDemo;
            if (cameraBeforeDemo) {
                sceneManager.setActiveCamera(cameraBeforeDemo);
                window.setCursorMode(cursorModeBeforeDemo);
            } else {
                sceneManager.setActiveCamera(sysCamera);
                window.setCursorMode(GLFW_CURSOR_NORMAL);
            }
        };

        auto renderCinematicOverlay = [&](const std::string& title, const std::string& explanation, const std::string& highlight, float alpha) {
            if (alpha <= 0.0f) return;
            
            int w_width = window.getWidth();
            int w_height = window.getHeight();
            
            glm::vec4 titleColor = glm::vec4(1.0f, 0.85f, 0.3f, alpha);
            glm::vec4 expColor = glm::vec4(0.9f, 0.9f, 0.9f, alpha);
            glm::vec4 highlightColor = glm::vec4(0.2f, 1.0f, 0.6f, alpha);

            // Title: Centered near the top
            float titleScale = 1.8f;
            float titleWidth = title.length() * 8.0f * titleScale;
            float titleX = (static_cast<float>(w_width) - titleWidth) / 2.0f;
            float titleY = w_height - 70.0f;
            textRenderer.renderText(title, titleX, titleY, titleScale, titleColor, w_width, w_height);

            // Wrap explanation text
            std::vector<std::string> lines;
            std::string currentLine = "";
            int maxCharsPerLine = 60;
            std::string word;
            std::stringstream ss(explanation);
            while (ss >> word) {
                if (currentLine.length() + word.length() + 1 > static_cast<size_t>(maxCharsPerLine)) {
                    lines.push_back(currentLine);
                    currentLine = word;
                } else {
                    if (!currentLine.empty()) currentLine += " ";
                    currentLine += word;
                }
            }
            if (!currentLine.empty()) {
                lines.push_back(currentLine);
            }

            float expScale = 1.15f;
            float startY = w_height - 105.0f;
            for (size_t i = 0; i < lines.size(); ++i) {
                float lineWidth = lines[i].length() * 8.0f * expScale;
                float lineX = (static_cast<float>(w_width) - lineWidth) / 2.0f;
                float lineY = startY - i * 20.0f;
                textRenderer.renderText(lines[i], lineX, lineY, expScale, expColor, w_width, w_height);
            }

            // Highlight target object name
            if (!highlight.empty()) {
                std::string hlText = "TARGET: " + highlight;
                float hlScale = 1.25f;
                float hlWidth = hlText.length() * 8.0f * hlScale;
                float hlX = (static_cast<float>(w_width) - hlWidth) / 2.0f;
                float hlY = startY - lines.size() * 20.0f - 22.0f;
                textRenderer.renderText(hlText, hlX, hlY, hlScale, highlightColor, w_width, w_height);
            }
        };

        // Main render loop
        while (!window.shouldClose()) {
            double currentTime = glfwGetTime();
            float deltaTime = static_cast<float>(currentTime - lastTime);
            lastTime = currentTime;

            GLFWwindow* glWin = window.getGLFWWindow();

            // Process ESC to exit demo mode
            if (currentDemo != DemoStage::NONE) {
                if (glfwGetKey(glWin, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
                    exitDemoMode();
                    glfwSetWindowShouldClose(glWin, GLFW_FALSE); // Prevent app closure
                }
            }

            // Keyboard selection: N for next planet, B for previous planet, ENTER to focus camera
            bool nIsPressed = (glfwGetKey(glWin, GLFW_KEY_N) == GLFW_PRESS);
            if (nIsPressed && !nWasPressed) {
                if (currentDemo != DemoStage::NONE) {
                    exitDemoMode();
                }
                selectedPlanetIdx = (selectedPlanetIdx + 1) % planets.size();
            }
            nWasPressed = nIsPressed;

            bool bIsPressed = (glfwGetKey(glWin, GLFW_KEY_B) == GLFW_PRESS);
            if (bIsPressed && !bWasPressed) {
                if (currentDemo != DemoStage::NONE) {
                    exitDemoMode();
                }
                selectedPlanetIdx = (selectedPlanetIdx - 1 + planets.size()) % planets.size();
            }
            bWasPressed = bIsPressed;

            bool enterIsPressed = (glfwGetKey(glWin, GLFW_KEY_ENTER) == GLFW_PRESS);
            if (enterIsPressed && !enterWasPressed) {
                if (currentDemo != DemoStage::NONE) {
                    exitDemoMode();
                }
                auto planet = planets[selectedPlanetIdx];
                float r = planet->getRadius();
                glm::vec3 offset = glm::vec3(0.0f, r * 2.0f, r * 4.0f);
                trackingCamera->setTargetPlanet(planet);
                trackingCamera->setOffset(offset);
                sceneManager.setActiveCamera(trackingCamera);
                window.setCursorMode(GLFW_CURSOR_NORMAL);
            }
            enterWasPressed = enterIsPressed;

            // Process camera switching input (only when demo is not active)
            if (currentDemo == DemoStage::NONE) {
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
                } else if (glfwGetKey(glWin, GLFW_KEY_5) == GLFW_PRESS) {
                    sceneManager.setActiveCamera(spacecraftCamera);
                }

                if (sceneManager.getActiveCamera() != oldCam) {
                    if (sceneManager.getActiveCamera() == freeCamera) {
                        window.setCursorMode(GLFW_CURSOR_DISABLED);
                    } else {
                        window.setCursorMode(GLFW_CURSOR_NORMAL);
                    }
                }
            }

            // Input handling for animation controls
            bool spaceIsPressed = (glfwGetKey(glWin, GLFW_KEY_SPACE) == GLFW_PRESS);
            if (currentDemo == DemoStage::NONE) {
                // Manual controls
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
            } else {
                // Demo active: SPACE pauses/resumes the demo
                if (spaceIsPressed && !spaceWasPressed) {
                    demoPaused = !demoPaused;
                    animationPaused = demoPaused; // Sync pause state
                }
                spaceWasPressed = spaceIsPressed;
            }

            // Demo mode F1 key detection
            bool f1IsPressed = (glfwGetKey(glWin, GLFW_KEY_F1) == GLFW_PRESS);
            if (f1IsPressed && !f1WasPressed) {
                if (currentDemo == DemoStage::NONE) {
                    // Start demo
                    cameraBeforeDemo = sceneManager.getActiveCamera();
                    cursorModeBeforeDemo = (cameraBeforeDemo == freeCamera) ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL;
                    speedBeforeDemo = animationSpeed;

                    if (cameraBeforeDemo) {
                        demoStartCamPos = cameraBeforeDemo->getPosition();
                        demoStartCamFront = cameraBeforeDemo->getFront();
                    } else {
                        demoStartCamPos = glm::vec3(0.0f, 6.5f, 9.5f);
                        demoStartCamFront = glm::vec3(0.0f, 0.0f, -1.0f);
                    }

                    currentDemo = DemoStage::OPENING_WIDE;
                    demoTimer = 0.0f;
                    demoPaused = false;
                    animationPaused = false;

                    sceneManager.setActiveCamera(sysCamera);
                    window.setCursorMode(GLFW_CURSOR_NORMAL);
                } else {
                    // Restart from beginning
                    demoTimer = 0.0f;
                    demoPaused = false;
                    animationPaused = false;

                    auto activeCam = sceneManager.getActiveCamera();
                    if (activeCam) {
                        demoStartCamPos = activeCam->getPosition();
                        demoStartCamFront = activeCam->getFront();
                    }
                }
            }
            f1WasPressed = f1IsPressed;

            // Timer update & auto-advance (only when demo is active and not paused)
            if (currentDemo != DemoStage::NONE) {
                if (!demoPaused) {
                    demoTimer += deltaTime;
                    if (demoTimer >= 180.0f) {
                        exitDemoMode();
                    }
                }
            }

            // Determine active sub-stage index
            if (currentDemo != DemoStage::NONE) {
                for (size_t i = 0; i < subStages.size(); ++i) {
                    if (demoTimer >= subStages[i].startTime && demoTimer < subStages[i].startTime + subStages[i].duration) {
                        currentSubStageIdx = static_cast<int>(i);
                        break;
                    }
                }
                currentDemo = subStages[currentSubStageIdx].stage;
                animationSpeed = subStages[currentSubStageIdx].speed;
            }

            // Clear screen
            renderer.clear(glm::vec4(0.04f, 0.04f, 0.06f, 1.0f));

            // Update active camera
            if (sceneManager.getActiveCamera()) {
                if (currentDemo == DemoStage::NONE) {
                    sceneManager.getActiveCamera()->update(deltaTime);
                } else {
                    // Dynamic camera update based on demo timeline
                    float stageTime = demoTimer - subStages[currentSubStageIdx].startTime;

                    glm::vec3 idealPos, idealLookAt;
                    getSubStageIdealState(currentSubStageIdx, demoTimer, idealPos, idealLookAt);

                    glm::vec3 finalPos = idealPos;
                    glm::vec3 finalLookAt = idealLookAt;

                    const float transitionDuration = 2.5f;
                    if (stageTime < transitionDuration) {
                        if (currentSubStageIdx > 0) {
                            float prevEndTime = subStages[currentSubStageIdx - 1].startTime + subStages[currentSubStageIdx - 1].duration;
                            glm::vec3 prevPos, prevLookAt;
                            getSubStageIdealState(currentSubStageIdx - 1, prevEndTime, prevPos, prevLookAt);

                            float u = stageTime / transitionDuration;
                            float s = u * u * (3.0f - 2.0f * u);
                            finalPos = glm::mix(prevPos, idealPos, s);
                            finalLookAt = glm::mix(prevLookAt, idealLookAt, s);
                        } else {
                            float u = stageTime / transitionDuration;
                            float s = u * u * (3.0f - 2.0f * u);
                            glm::vec3 prevPos = demoStartCamPos;
                            glm::vec3 prevLookAt = demoStartCamPos + demoStartCamFront * 5.0f;
                            finalPos = glm::mix(prevPos, idealPos, s);
                            finalLookAt = glm::mix(prevLookAt, idealLookAt, s);
                        }
                    }

                    auto activeCam = sceneManager.getActiveCamera();
                    if (activeCam) {
                        activeCam->setPosition(finalPos);
                        if (glm::length(finalLookAt - finalPos) > 0.0001f) {
                            activeCam->setFront(glm::normalize(finalLookAt - finalPos));
                        }
                        glm::vec3 worldUp = glm::vec3(0.0f, 1.0f, 0.0f);
                        glm::vec3 right = glm::normalize(glm::cross(activeCam->getFront(), worldUp));
                        activeCam->setUp(glm::normalize(glm::cross(right, activeCam->getFront())));
                    }
                }
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

            // Mission telemetry HUD (Only in manual mode, or when Spacecraft highlight is active)
            bool showSpacecraftTelemetry = (currentDemo == DemoStage::NONE) || 
                (currentDemo != DemoStage::NONE && subStages[currentSubStageIdx].highlight == "Spacecraft");
            if (spacecraft && showSpacecraftTelemetry) {
                float progress = spacecraft->getProgress();
                char progressPercentStr[32];
                std::snprintf(progressPercentStr, sizeof(progressPercentStr), "%.1f%%", progress * 100.0f);
                
                int barWidth = 12;
                int filled = static_cast<int>(progress * barWidth);
                std::string bar = " [";
                for (int i = 0; i < barWidth; ++i) {
                    if (i < filled) bar += "=";
                    else if (i == filled) bar += ">";
                    else bar += ".";
                }
                bar += "]";

                textRenderer.renderText("Mission: Earth to Mars", 15.0f, height - 85.0f, 1.2f, glm::vec3(0.0f, 0.9f, 1.0f), width, height);
                textRenderer.renderText("Current target: Mars", 15.0f, height - 105.0f, 1.2f, glm::vec3(0.0f, 0.9f, 1.0f), width, height);
                textRenderer.renderText(std::string("Progress: ") + progressPercentStr + bar, 15.0f, height - 125.0f, 1.2f, glm::vec3(0.0f, 0.9f, 1.0f), width, height);
            }

            // Cinematic Overlay for Demo Mode
            if (currentDemo != DemoStage::NONE) {
                float stageTime = demoTimer - subStages[currentSubStageIdx].startTime;
                float duration = subStages[currentSubStageIdx].duration;
                float alpha = 1.0f;
                if (stageTime < 1.5f) {
                    alpha = stageTime / 1.5f;
                } else if (stageTime > duration - 1.5f) {
                    alpha = (duration - stageTime) / 1.5f;
                }
                alpha = glm::clamp(alpha, 0.0f, 1.0f);

                renderCinematicOverlay(subStages[currentSubStageIdx].title, subStages[currentSubStageIdx].explanation, subStages[currentSubStageIdx].highlight, alpha);
            }

            // BOTTOM-LEFT block — Controls:
            if (currentDemo == DemoStage::NONE) {
                textRenderer.renderText("[1-5] Cameras  [N/B] Next/Prev Planet  [ENTER] Focus", 15.0f, 75.0f, 1.1f, glm::vec3(1.0f, 1.0f, 1.0f), width, height);
                textRenderer.renderText("[SPACE] Pause  [+/-] Speed  [F1] Demo Mode", 15.0f, 50.0f, 1.1f, glm::vec3(1.0f, 1.0f, 1.0f), width, height);
                textRenderer.renderText("[ESC] Exit", 15.0f, 25.0f, 1.1f, glm::vec3(0.5f, 0.5f, 0.5f), width, height);
            } else {
                textRenderer.renderText("[SPACE] Pause/Resume Demo", 15.0f, 55.0f, 1.1f, glm::vec3(1.0f, 1.0f, 1.0f), width, height);
                textRenderer.renderText("[F1] Restart Demo", 15.0f, 35.0f, 1.1f, glm::vec3(0.2f, 1.0f, 0.6f), width, height);
                textRenderer.renderText("[ESC] Exit Demo", 15.0f, 15.0f, 1.1f, glm::vec3(0.5f, 0.5f, 0.5f), width, height);
            }

            // BOTTOM-CENTER — PAUSED indicator (only when paused):
            if (animationPaused) {
                std::string pauseText = (currentDemo == DemoStage::NONE) ? "⏸ PAUSED" : "⏸ DEMO PAUSED";
                float pauseScale = 2.0f;
                float offset = (pauseText.length() * 8.0f * pauseScale) / 2.0f;
                glm::vec3 pauseColor = (currentDemo == DemoStage::NONE) ? glm::vec3(1.0f, 0.5f, 0.0f) : glm::vec3(1.0f, 0.35f, 0.0f);
                textRenderer.renderText(pauseText, (width / 2.0f) - offset, 35.0f, pauseScale, pauseColor, width, height);
            }

            // TOP-RIGHT — Speed and camera info:
            std::string camName = sceneManager.getActiveCamera() ? sceneManager.getActiveCamera()->getName() : "None";
            std::string camStr = "Camera: " + camName;
            if (currentDemo != DemoStage::NONE) {
                camStr += " (Demo)";
            }
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
                if (currentDemo != DemoStage::NONE) {
                    speedOverlayStr += " (Demo)";
                }
                if (animationSpeed > 1.5f) {
                    speedColor = glm::vec3(1.0f, 1.0f, 0.0f);
                }
            }
            float speedScale = 1.3f;
            float speedX = width - (speedOverlayStr.length() * 8.0f * speedScale) - 15.0f;
            textRenderer.renderText(speedOverlayStr, speedX, height - 50.0f, speedScale, speedColor, width, height);

            // Render label overlay if camera is near Earth (only in manual mode or specific stages)
            if (sceneManager.getActiveCamera() && earth) {
                glm::vec3 camPos = sceneManager.getActiveCamera()->getPosition();
                glm::vec3 earthPos = earth->getPosition();
                float dist = glm::distance(camPos, earthPos);
                // In demo mode, only show if target matches SpaceStation
                bool showStationLabel = (currentDemo == DemoStage::NONE && dist < 2.0f) || 
                    (currentDemo != DemoStage::NONE && subStages[currentSubStageIdx].highlight == "SpaceStation");
                if (showStationLabel) {
                    std::string labelText = "ISS-style Space Station";
                    float labelScale = 1.3f;
                    float labelWidth = labelText.length() * 8.0f * labelScale;
                    float labelX = (width - labelWidth) / 2.0f;
                    float labelY = height - 90.0f; 
                    textRenderer.renderText(labelText, labelX, labelY, labelScale, glm::vec3(0.0f, 0.9f, 1.0f), width, height);
                }
            }

            // --- RIGHT-SIDE INFORMATION OVERLAY HUD PANEL ---
            bool showHudPanel = false;
            PlanetInfo hudInfo;
            bool isDemoFocus = false;

            if (currentDemo == DemoStage::NONE) {
                // Manual mode: show selected planet details
                if (selectedPlanetIdx >= 0 && selectedPlanetIdx < static_cast<int>(planets.size())) {
                    hudInfo = getPlanetInfo(planets[selectedPlanetIdx]->getName());
                    showHudPanel = true;
                    isDemoFocus = false;
                }
            } else {
                // Demo mode: show current demo highlight if not empty
                std::string hl = subStages[currentSubStageIdx].highlight;
                if (!hl.empty()) {
                    hudInfo = getPlanetInfo(hl);
                    showHudPanel = true;
                    isDemoFocus = true;
                }
            }

            if (showHudPanel) {
                float panelW = 320.0f;
                float panelH = 430.0f;
                float panelX = width - panelW - 15.0f;
                float panelY = height - panelH - 80.0f; // fits nicely below top-right speed/camera text

                // 1. Draw glowing 1px border (green for demo focus, cyan for manual focus)
                glm::vec4 borderColor = isDemoFocus ? glm::vec4(0.2f, 1.0f, 0.6f, 0.4f) : glm::vec4(0.0f, 0.8f, 1.0f, 0.4f);
                textRenderer.renderQuad(panelX - 1.0f, panelY - 1.0f, panelW + 2.0f, panelH + 2.0f, borderColor, width, height);

                // 2. Draw dark semi-transparent background
                glm::vec4 bgCol = glm::vec4(0.06f, 0.08f, 0.12f, 0.75f);
                textRenderer.renderQuad(panelX, panelY, panelW, panelH, bgCol, width, height);

                // 3. Draw panel content
                float curY = panelY + panelH - 25.0f;
                
                // Mode Tag [ SELECTED ] or [ DEMO FOCUS ]
                if (isDemoFocus) {
                    textRenderer.renderText("[ DEMO TARGET ]", panelX + 15.0f, curY, 0.9f, glm::vec3(0.2f, 1.0f, 0.6f), width, height);
                } else {
                    textRenderer.renderText("[ SELECTED ]", panelX + 15.0f, curY, 0.9f, glm::vec3(0.0f, 0.8f, 1.0f), width, height);
                }
                curY -= 25.0f;

                // Planet / Target Name
                textRenderer.renderText(hudInfo.name, panelX + 15.0f, curY, 1.6f, glm::vec3(1.0f, 0.85f, 0.3f), width, height);
                curY -= 15.0f;

                // Separator Line
                textRenderer.renderQuad(panelX + 15.0f, curY, panelW - 30.0f, 2.0f, borderColor, width, height);
                curY -= 20.0f;

                // Text wrapping lambda to fit values nicely inside the panel
                auto renderWrappedText = [&](const std::string& text, float x, float y, float maxW, float scale, const glm::vec3& color) {
                    std::vector<std::string> lines;
                    std::string currentLine = "";
                    std::stringstream ss(text);
                    std::string word;
                    float charW = 8.0f * scale;
                    while (ss >> word) {
                        std::string testLine = currentLine.empty() ? word : currentLine + " " + word;
                        if (testLine.length() * charW > maxW) {
                            lines.push_back(currentLine);
                            currentLine = word;
                        } else {
                            currentLine = testLine;
                        }
                    }
                    if (!currentLine.empty()) {
                        lines.push_back(currentLine);
                    }
                    
                    float lineY = y;
                    for (const auto& line : lines) {
                        textRenderer.renderText(line, x, lineY, scale, color, width, height);
                        lineY -= 15.0f * scale;
                    }
                    return y - lineY; // returns height consumed
                };

                // Type Section
                textRenderer.renderText("TYPE", panelX + 15.0f, curY, 1.0f, glm::vec3(0.0f, 0.8f, 1.0f), width, height);
                curY -= 18.0f;
                float typeH = renderWrappedText(hudInfo.type, panelX + 15.0f, curY, panelW - 30.0f, 1.0f, glm::vec3(0.9f, 0.9f, 0.9f));
                curY -= typeH + 12.0f;

                // Features Section
                textRenderer.renderText("FEATURES", panelX + 15.0f, curY, 1.0f, glm::vec3(0.0f, 0.8f, 1.0f), width, height);
                curY -= 18.0f;
                float featH = renderWrappedText(hudInfo.features, panelX + 15.0f, curY, panelW - 30.0f, 1.0f, glm::vec3(0.9f, 0.9f, 0.9f));
                curY -= featH + 12.0f;

                // Moons Section
                textRenderer.renderText("MOONS INCLUDED", panelX + 15.0f, curY, 1.0f, glm::vec3(0.0f, 0.8f, 1.0f), width, height);
                curY -= 18.0f;
                float moonH = renderWrappedText(hudInfo.moons, panelX + 15.0f, curY, panelW - 30.0f, 1.0f, glm::vec3(0.9f, 0.9f, 0.9f));
                curY -= moonH + 12.0f;

                // Concepts Section
                textRenderer.renderText("CG CONCEPTS DEMONSTRATED", panelX + 15.0f, curY, 1.0f, glm::vec3(0.0f, 0.8f, 1.0f), width, height);
                curY -= 18.0f;
                renderWrappedText(hudInfo.concepts, panelX + 15.0f, curY, panelW - 30.0f, 1.0f, glm::vec3(0.9f, 0.9f, 0.9f));
            }

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
                    demoSuffix = " | Demo: " + subStages[currentSubStageIdx].name;
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
