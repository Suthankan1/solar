#include "core/Window.h"
#include "core/Renderer.h"
#include "scene/SceneManager.h"
#include "celestial/Sun.h"
#include "celestial/SunHalo.h"
#include "celestial/Planet.h"
#include "celestial/Moon.h"
#include "celestial/Orbit.h"
#include "celestial/OrbitMath.h"
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
#include <algorithm>
#include <cstdio>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

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

float UI_SCALE = 1.35f;

namespace {
enum class OrbitQuality {
    Low,
    High
};

struct PlanetLayout {
    float radius;
    float semiMajorAxis;
    float semiMinorAxis;
};

// Visual scale model:
// - Planet radii are enlarged for teaching readability, while solar orbit
//   distances remain compressed enough to keep all planets in one scene.
// - Local systems use their own readable scale: moon/station orbit radii are
//   chosen from visual clearance first, not astronomical distance ratios.
// - All starting phases are deterministic so the opening frame is repeatable
//   and validation can catch bad placements reliably.
constexpr PlanetLayout kMercuryLayout{0.20f, 2.0f, 1.75f};
constexpr PlanetLayout kVenusLayout{0.32f, 2.9f, 2.86f};
constexpr PlanetLayout kEarthLayout{0.42f, 4.2f, 4.16f};
constexpr PlanetLayout kMarsLayout{0.32f, 5.9f, 5.64f};
constexpr PlanetLayout kJupiterLayout{1.05f, 10.2f, 10.0f};
constexpr PlanetLayout kSaturnLayout{0.90f, 14.1f, 13.85f};
constexpr PlanetLayout kUranusLayout{0.62f, 18.2f, 17.9f};
constexpr PlanetLayout kNeptuneLayout{0.58f, 21.3f, 21.0f};

constexpr float kAsteroidBeltInnerRadius = 7.25f;
constexpr float kAsteroidBeltOuterRadius = 8.15f;

constexpr float kEarthCloudScale = 1.015f;
constexpr float kEarthAtmosphereScale = 1.035f;
constexpr float kEarthMoonRadius = 0.13f;
constexpr float kEarthMoonOrbitRadius = 1.18f;
constexpr float kEarthMoonOrbitInclination = 5.1f;
constexpr float kEarthMoonOrbitNode = 18.0f;
constexpr float kEarthMoonPhase = 2.2f;

constexpr float kSpaceStationOrbitRadius = 0.70f;
constexpr float kSpaceStationOrbitInclination = 51.6f;
constexpr float kSpaceStationOrbitNode = 0.0f;
constexpr float kSpaceStationPhase = 5.0f;
constexpr float kSpaceStationBoundsRadius = 0.18f;

constexpr float kPhobosRadius = 0.07f;
constexpr float kPhobosOrbitRadius = 0.56f;
constexpr float kPhobosOrbitInclination = 1.0f;
constexpr float kPhobosOrbitNode = 70.0f;
constexpr float kPhobosPhase = 1.4f;

constexpr float kIoRadius = 0.15f;
constexpr float kIoOrbitRadius = 1.65f;
constexpr float kIoOrbitInclination = 2.1f;
constexpr float kIoOrbitNode = 35.0f;
constexpr float kIoPhase = 0.7f;

constexpr float kEuropaRadius = 0.12f;
constexpr float kEuropaOrbitRadius = 2.25f;
constexpr float kEuropaOrbitInclination = -3.3f;
constexpr float kEuropaOrbitNode = 118.0f;
constexpr float kEuropaPhase = 3.3f;

struct BodyBounds {
    std::string name;
    glm::vec3 position;
    float radius;
};

bool wouldOverlap(const glm::vec3& posA,
                  float radiusA,
                  const glm::vec3& posB,
                  float radiusB,
                  float margin) {
    return glm::distance(posA, posB) < (radiusA + radiusB + margin);
}

const char* asteroidQualityName(AsteroidBelt::Quality quality) {
    switch (quality) {
        case AsteroidBelt::Quality::Low: return "LOW";
        case AsteroidBelt::Quality::Medium: return "MED";
        case AsteroidBelt::Quality::High: return "HIGH";
    }
    return "HIGH";
}

#ifndef NDEBUG
void warnLayout(const std::string& message) {
    std::cerr << "Layout Warning: " << message << '\n';
}

void validateOrbitClearance(const std::string& childName,
                            float childRadius,
                            const std::string& parentName,
                            float parentRadius,
                            float orbitRadius,
                            float margin) {
    const float required = parentRadius + childRadius + margin;
    if (orbitRadius < required) {
        warnLayout(childName + " orbit around " + parentName +
                   " is too tight (orbit=" + std::to_string(orbitRadius) +
                   ", required=" + std::to_string(required) + ").");
    }
}

void validateAdjacentOrbitGap(const std::string& innerName,
                              const PlanetLayout& inner,
                              const std::string& outerName,
                              const PlanetLayout& outer,
                              float margin) {
    const float gap = outer.semiMinorAxis - inner.semiMajorAxis;
    const float required = inner.radius + outer.radius + margin;
    if (gap < required) {
        warnLayout(innerName + " and " + outerName +
                   " orbital lanes are too close (gap=" + std::to_string(gap) +
                   ", required=" + std::to_string(required) + ").");
    }
}

void validateStartupOverlaps(const std::vector<BodyBounds>& bodies, float margin) {
    for (std::size_t i = 0; i < bodies.size(); ++i) {
        for (std::size_t j = i + 1; j < bodies.size(); ++j) {
            if (wouldOverlap(bodies[i].position, bodies[i].radius, bodies[j].position, bodies[j].radius, margin)) {
                warnLayout(bodies[i].name + " starts too close to " + bodies[j].name + ".");
            }
        }
    }
}
#endif
}

// Project 3D world position to 2D screen position
static bool projectWorldToScreen(const glm::vec3& worldPos, const glm::mat4& view, const glm::mat4& projection, int width, int height, glm::vec2& screenPos) {
    glm::vec4 clipSpacePos = projection * view * glm::vec4(worldPos, 1.0f);
    if (clipSpacePos.w <= 0.0f) return false; // Behind camera
    
    glm::vec3 ndcSpacePos = glm::vec3(clipSpacePos) / clipSpacePos.w;
    
    // Map NDC [-1, 1] to screen coordinates [0, width] and [0, height]
    screenPos.x = (ndcSpacePos.x + 1.0f) * 0.5f * width;
    screenPos.y = (ndcSpacePos.y + 1.0f) * 0.5f * height; // Y is 0 at bottom in TextRenderer
    return true;
}

int main() {
    try {
        std::cout << "Starting Modern OpenGL Solar System Application...\n";

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
        // Create Planets (name, radius, semiMajorAxis, semiMinorAxis, inclination, orbitSpeed, rotationSpeed, color, texturePath, orbitPhaseOffset, longitudeOfAscendingNode)
        auto mercury = std::make_shared<Planet>("Mercury", kMercuryLayout.radius, kMercuryLayout.semiMajorAxis, kMercuryLayout.semiMinorAxis, 7.0f, 1.6f, 3.0f, glm::vec3(0.76f, 0.70f, 0.65f), "textures/mercury.jpg", 0.5f, 48.3f);
        auto venus = std::make_shared<Planet>("Venus", kVenusLayout.radius, kVenusLayout.semiMajorAxis, kVenusLayout.semiMinorAxis, 3.4f, 1.1f, 0.4f, glm::vec3(0.95f, 0.85f, 0.55f), "textures/venus.jpg", 1.2f, 76.7f);
        auto earth = std::make_shared<Planet>("Earth", kEarthLayout.radius, kEarthLayout.semiMajorAxis, kEarthLayout.semiMinorAxis, 0.0f, 0.9f, 1.8f, glm::vec3(0.25f, 0.55f, 0.95f), "textures/earth.jpg", 0.0f, 0.0f);
        auto mars = std::make_shared<Planet>("Mars", kMarsLayout.radius, kMarsLayout.semiMajorAxis, kMarsLayout.semiMinorAxis, 1.85f, 0.7f, 1.7f, glm::vec3(0.80f, 0.35f, 0.20f), "textures/mars.jpg", 2.4f, 49.6f);
        auto asteroidBelt = std::make_shared<AsteroidBelt>("AsteroidBelt", kAsteroidBeltInnerRadius, kAsteroidBeltOuterRadius, 2000);
        auto jupiter = std::make_shared<Planet>("Jupiter", kJupiterLayout.radius, kJupiterLayout.semiMajorAxis, kJupiterLayout.semiMinorAxis, 1.3f, 0.4f, 2.5f, glm::vec3(0.85f, 0.72f, 0.58f), "textures/jupiter.jpg", 3.8f, 100.5f);
        auto saturn = std::make_shared<Planet>("Saturn", kSaturnLayout.radius, kSaturnLayout.semiMajorAxis, kSaturnLayout.semiMinorAxis, 2.48f, 0.3f, 2.2f, glm::vec3(0.92f, 0.86f, 0.65f), "textures/saturn.jpg", 4.5f, 113.7f);
        auto saturnRings = std::make_shared<SaturnRings>("SaturnRings", saturn);
        auto uranus = std::make_shared<Planet>("Uranus", kUranusLayout.radius, kUranusLayout.semiMajorAxis, kUranusLayout.semiMinorAxis, 0.77f, 0.2f, 1.4f, glm::vec3(0.55f, 0.85f, 0.90f), "textures/uranus.jpg", 5.1f, 74.0f);
        auto neptune = std::make_shared<Planet>("Neptune", kNeptuneLayout.radius, kNeptuneLayout.semiMajorAxis, kNeptuneLayout.semiMinorAxis, 1.77f, 0.15f, 1.5f, glm::vec3(0.25f, 0.40f, 0.95f), "textures/neptune.jpg", 1.8f, 131.8f);

        // Create Orbit Rings (name + "Orbit", planet, color, parent)
        auto mercuryOrbit = std::make_shared<Orbit>("MercuryOrbit", mercury, glm::vec3(0.4f, 0.38f, 0.36f), nullptr);
        auto venusOrbit = std::make_shared<Orbit>("VenusOrbit", venus, glm::vec3(0.5f, 0.45f, 0.30f), nullptr);
        auto earthOrbit = std::make_shared<Orbit>("EarthOrbit", earth, glm::vec3(0.2f, 0.5f, 0.9f), nullptr);
        auto marsOrbit = std::make_shared<Orbit>("MarsOrbit", mars, glm::vec3(0.7f, 0.25f, 0.1f), nullptr);
        auto jupiterOrbit = std::make_shared<Orbit>("JupiterOrbit", jupiter, glm::vec3(0.45f, 0.38f, 0.30f), nullptr);
        auto saturnOrbit = std::make_shared<Orbit>("SaturnOrbit", saturn, glm::vec3(0.48f, 0.44f, 0.33f), nullptr);
        auto uranusOrbit = std::make_shared<Orbit>("UranusOrbit", uranus, glm::vec3(0.28f, 0.42f, 0.46f), nullptr);
        auto neptuneOrbit = std::make_shared<Orbit>("NeptuneOrbit", neptune, glm::vec3(0.12f, 0.20f, 0.48f), nullptr);
        auto moonOrbit = std::make_shared<Orbit>(
            "MoonOrbit",
            kEarthMoonOrbitRadius,
            kEarthMoonOrbitRadius,
            kEarthMoonOrbitInclination,
            kEarthMoonOrbitNode,
            glm::vec3(0.25f, 0.28f, 0.38f),
            earth
        );
        moonOrbit->setLocalOrbitStyle(0.0f, glm::vec3(0.42f, 0.50f, 0.72f), 1.05f);
        auto phobosOrbit = std::make_shared<Orbit>(
            "PhobosOrbit",
            kPhobosOrbitRadius,
            kPhobosOrbitRadius,
            kPhobosOrbitInclination,
            kPhobosOrbitNode,
            glm::vec3(0.55f, 0.36f, 0.28f),
            mars
        );
        phobosOrbit->setLocalOrbitStyle(0.0f, glm::vec3(0.70f, 0.42f, 0.32f), 0.85f);
        auto ioOrbit = std::make_shared<Orbit>(
            "IoOrbit",
            kIoOrbitRadius,
            kIoOrbitRadius,
            kIoOrbitInclination,
            kIoOrbitNode,
            glm::vec3(0.70f, 0.62f, 0.30f),
            jupiter
        );
        ioOrbit->setLocalOrbitStyle(0.0f, glm::vec3(0.88f, 0.74f, 0.34f), 0.85f);
        auto europaOrbit = std::make_shared<Orbit>(
            "EuropaOrbit",
            kEuropaOrbitRadius,
            kEuropaOrbitRadius,
            kEuropaOrbitInclination,
            kEuropaOrbitNode,
            glm::vec3(0.50f, 0.56f, 0.68f),
            jupiter
        );
        europaOrbit->setLocalOrbitStyle(0.0f, glm::vec3(0.62f, 0.70f, 0.85f), 0.82f);

        // Create Moons (name, radius, orbitRadius, orbitSpeed, rotationSpeed, color, parentPlanet)
        auto moon = std::make_shared<Moon>("Moon", kEarthMoonRadius, kEarthMoonOrbitRadius, 2.5f, 1.2f, glm::vec3(0.75f, 0.75f, 0.72f), earth, "textures/moon.jpg", kEarthMoonPhase, kEarthMoonOrbitInclination, kEarthMoonOrbitNode);
        auto phobos = std::make_shared<Moon>("Phobos", kPhobosRadius, kPhobosOrbitRadius, 5.0f, 2.0f, glm::vec3(0.60f, 0.55f, 0.48f), mars, "", kPhobosPhase, kPhobosOrbitInclination, kPhobosOrbitNode);
        auto io = std::make_shared<Moon>("Io", kIoRadius, kIoOrbitRadius, 3.0f, 1.0f, glm::vec3(0.95f, 0.88f, 0.45f), jupiter, "", kIoPhase, kIoOrbitInclination, kIoOrbitNode);
        auto europa = std::make_shared<Moon>("Europa", kEuropaRadius, kEuropaOrbitRadius, 2.2f, 1.2f, glm::vec3(0.80f, 0.75f, 0.70f), jupiter, "", kEuropaPhase, kEuropaOrbitInclination, kEuropaOrbitNode);

        // Create Space Station and its Orbit orbiting Earth
        auto spaceStation = std::make_shared<SpaceStation>("SpaceStation", kSpaceStationOrbitRadius, 2.2f, earth, kSpaceStationPhase, kSpaceStationOrbitInclination, kSpaceStationOrbitNode);
        auto spaceStationOrbit = std::make_shared<Orbit>("SpaceStationOrbit", kSpaceStationOrbitRadius, kSpaceStationOrbitRadius, kSpaceStationOrbitInclination, kSpaceStationOrbitNode, glm::vec3(0.2f, 0.35f, 0.45f), earth);
        spaceStationOrbit->setLocalOrbitStyle(0.0f, glm::vec3(0.25f, 0.78f, 0.95f), 1.15f);

        // Create Spacecraft
        auto spacecraft = std::make_shared<Spacecraft>("Spacecraft", earth, mars);

        // Instantiate and register Earth's transparent layers (clouds & atmosphere)
        auto earthClouds = std::make_shared<CloudLayer>("EarthClouds", earth, kEarthCloudScale, 1.95f);
        auto earthAtmosphere = std::make_shared<AtmosphereLayer>("EarthAtmosphere", earth, kEarthAtmosphereScale, glm::vec3(0.25f, 0.55f, 1.0f));

        // 3. Create Skybox Background
        auto skybox = std::make_shared<Skybox>("Skybox");
        auto starfield = std::make_shared<Starfield>("Starfield", 3000, 80.0f);

        mercury->update(0.0f);
        venus->update(0.0f);
        earth->update(0.0f);
        mars->update(0.0f);
        jupiter->update(0.0f);
        saturn->update(0.0f);
        uranus->update(0.0f);
        neptune->update(0.0f);
        moon->update(0.0f);
        phobos->update(0.0f);
        io->update(0.0f);
        europa->update(0.0f);
        spaceStation->update(0.0f);
        spacecraft->update(0.0f);
        mercuryOrbit->update(0.0f);
        venusOrbit->update(0.0f);
        earthOrbit->update(0.0f);
        marsOrbit->update(0.0f);
        jupiterOrbit->update(0.0f);
        saturnOrbit->update(0.0f);
        uranusOrbit->update(0.0f);
        neptuneOrbit->update(0.0f);
        moonOrbit->update(0.0f);
        phobosOrbit->update(0.0f);
        ioOrbit->update(0.0f);
        europaOrbit->update(0.0f);
        spaceStationOrbit->update(0.0f);
        saturnRings->update(0.0f);
        earthClouds->update(0.0f);
        earthAtmosphere->update(0.0f);

#ifndef NDEBUG
        validateOrbitClearance("Mercury", kMercuryLayout.radius, "Sun", 1.2f, kMercuryLayout.semiMinorAxis, 0.20f);
        validateOrbitClearance("Venus", kVenusLayout.radius, "Sun", 1.2f, kVenusLayout.semiMinorAxis, 0.20f);
        validateOrbitClearance("Earth", kEarthLayout.radius, "Sun", 1.2f, kEarthLayout.semiMinorAxis, 0.20f);
        validateOrbitClearance("Mars", kMarsLayout.radius, "Sun", 1.2f, kMarsLayout.semiMinorAxis, 0.20f);
        validateOrbitClearance("Jupiter", kJupiterLayout.radius, "Sun", 1.2f, kJupiterLayout.semiMinorAxis, 0.20f);
        validateOrbitClearance("Saturn", kSaturnLayout.radius, "Sun", 1.2f, kSaturnLayout.semiMinorAxis, 0.20f);
        validateOrbitClearance("Uranus", kUranusLayout.radius, "Sun", 1.2f, kUranusLayout.semiMinorAxis, 0.20f);
        validateOrbitClearance("Neptune", kNeptuneLayout.radius, "Sun", 1.2f, kNeptuneLayout.semiMinorAxis, 0.20f);

        validateAdjacentOrbitGap("Mercury", kMercuryLayout, "Venus", kVenusLayout, 0.25f);
        validateAdjacentOrbitGap("Venus", kVenusLayout, "Earth", kEarthLayout, 0.28f);
        validateAdjacentOrbitGap("Earth", kEarthLayout, "Mars", kMarsLayout, 0.30f);
        validateAdjacentOrbitGap("Mars", kMarsLayout, "Jupiter", kJupiterLayout, 0.55f);
        validateAdjacentOrbitGap("Jupiter", kJupiterLayout, "Saturn", kSaturnLayout, 0.55f);
        validateAdjacentOrbitGap("Saturn", kSaturnLayout, "Uranus", kUranusLayout, 0.30f);
        validateAdjacentOrbitGap("Uranus", kUranusLayout, "Neptune", kNeptuneLayout, 0.30f);

        const float earthVisualRadius = kEarthLayout.radius * kEarthAtmosphereScale;
        validateOrbitClearance("Moon", kEarthMoonRadius, "Earth atmosphere", earthVisualRadius, kEarthMoonOrbitRadius, 0.08f);
        validateOrbitClearance("SpaceStation", kSpaceStationBoundsRadius, "Earth atmosphere", earthVisualRadius, kSpaceStationOrbitRadius, 0.06f);
        validateOrbitClearance("Phobos", kPhobosRadius, "Mars", kMarsLayout.radius, kPhobosOrbitRadius, 0.07f);
        validateOrbitClearance("Io", kIoRadius, "Jupiter", kJupiterLayout.radius, kIoOrbitRadius, 0.12f);
        validateOrbitClearance("Europa", kEuropaRadius, "Jupiter", kJupiterLayout.radius, kEuropaOrbitRadius, 0.12f);

        if ((kAsteroidBeltInnerRadius - (kMarsLayout.semiMajorAxis + kMarsLayout.radius)) < 0.55f) {
            warnLayout("Asteroid belt inner edge is too close to Mars.");
        }
        if (((kJupiterLayout.semiMinorAxis - kJupiterLayout.radius) - kAsteroidBeltOuterRadius) < 0.55f) {
            warnLayout("Asteroid belt outer edge is too close to Jupiter.");
        }
        const float saturnRingOuterRadius = kSaturnLayout.radius * SaturnRings::kOuterRadiusMultiplier;
        const float saturnToUranusGap = kUranusLayout.semiMinorAxis - kSaturnLayout.semiMajorAxis;
        if (saturnToUranusGap < saturnRingOuterRadius + kUranusLayout.radius + 0.30f) {
            warnLayout("Saturn ring outer edge is too close to Uranus orbital lane.");
        }

        validateStartupOverlaps({
            {"Sun", sun->getPosition(), 1.2f},
            {"Mercury", mercury->getPosition(), mercury->getRadius()},
            {"Venus", venus->getPosition(), venus->getRadius()},
            {"Earth", earth->getPosition(), earthVisualRadius},
            {"Moon", moon->getPosition(), moon->getRadius()},
            {"SpaceStation", spaceStation->getPosition(), kSpaceStationBoundsRadius},
            {"Mars", mars->getPosition(), mars->getRadius()},
            {"Phobos", phobos->getPosition(), phobos->getRadius()},
            {"Jupiter", jupiter->getPosition(), jupiter->getRadius()},
            {"Io", io->getPosition(), io->getRadius()},
            {"Europa", europa->getPosition(), europa->getRadius()},
            {"Saturn", saturn->getPosition(), saturn->getRadius()},
            {"Uranus", uranus->getPosition(), uranus->getRadius()},
            {"Neptune", neptune->getPosition(), neptune->getRadius()}
        }, 0.06f);
#endif

        // Register in render-friendly order: background, opaque bodies, then
        // transparent orbit/effect layers. This keeps alpha-blended visuals
        // from producing false overlap artifacts while preserving current
        // planet positions for local orbit rings during update.
        sceneManager.registerObject(skybox);
        sceneManager.registerObject(starfield);
        sceneManager.registerObject(sun);
        sceneManager.registerObject(mercury);
        sceneManager.registerObject(venus);
        sceneManager.registerObject(earth);
        sceneManager.registerObject(mars);
        sceneManager.registerObject(asteroidBelt);
        sceneManager.registerObject(jupiter);
        sceneManager.registerObject(saturn);
        sceneManager.registerObject(uranus);
        sceneManager.registerObject(neptune);
        sceneManager.registerObject(moon);
        sceneManager.registerObject(spaceStation);
        sceneManager.registerObject(spacecraft);
        sceneManager.registerObject(phobos);
        sceneManager.registerObject(io);
        sceneManager.registerObject(europa);

        sceneManager.registerObject(mercuryOrbit);
        sceneManager.registerObject(venusOrbit);
        sceneManager.registerObject(earthOrbit);
        sceneManager.registerObject(marsOrbit);
        sceneManager.registerObject(jupiterOrbit);
        sceneManager.registerObject(saturnOrbit);
        sceneManager.registerObject(uranusOrbit);
        sceneManager.registerObject(neptuneOrbit);
        sceneManager.registerObject(moonOrbit);
        sceneManager.registerObject(spaceStationOrbit);
        sceneManager.registerObject(phobosOrbit);
        sceneManager.registerObject(ioOrbit);
        sceneManager.registerObject(europaOrbit);
        sceneManager.registerObject(saturnRings);
        sceneManager.registerObject(earthClouds);
        sceneManager.registerObject(earthAtmosphere);
        sceneManager.registerObject(sunHalo);

        // 4. Create and Register Cameras
        auto sysCamera = std::make_shared<StaticCamera>("SystemCamera", glm::vec3(0.0f, 9.5f, 18.0f), glm::vec3(0.0f, 0.0f, 0.0f));
        auto freeCamera = std::make_shared<FreeCamera>("FreeCamera", glm::vec3(0.0f, 5.0f, 12.0f));
        auto spacecraftCamera = std::make_shared<SpacecraftFollowCamera>("Spacecraft Follow", spacecraft, 0.08f, 0.025f);
        auto trackingCamera = std::make_shared<TrackingCamera>("Planet Focus", earth, glm::vec3(0.0f, 0.36f, 0.72f));

        sceneManager.setActiveCamera(sysCamera);
        sceneManager.registerObject(spacecraftCamera);
        sceneManager.registerObject(trackingCamera);
        window.setCursorMode(GLFW_CURSOR_NORMAL);

        // Vector of all planets for keyboard selection cycling
        std::vector<std::shared_ptr<Planet>> planets = { mercury, venus, earth, mars, jupiter, saturn, uranus, neptune };
        std::vector<std::shared_ptr<Orbit>> solarOrbits = { mercuryOrbit, venusOrbit, earthOrbit, marsOrbit, jupiterOrbit, saturnOrbit, uranusOrbit, neptuneOrbit };
        std::vector<std::shared_ptr<Orbit>> allOrbits = {
            mercuryOrbit, venusOrbit, earthOrbit, marsOrbit, jupiterOrbit, saturnOrbit, uranusOrbit, neptuneOrbit,
            moonOrbit, spaceStationOrbit, phobosOrbit, ioOrbit, europaOrbit
        };

        std::cout << "Scene Manager initialized: Objects and cameras registered successfully.\n";
        std::cout << "Press keys [1], [2], [3], or [4] to switch cameras:\n";
        std::cout << "  [1] Global System View (Static Camera)\n";
        std::cout << "  [2] Top-Down View (Static Camera)\n";
        std::cout << "  [3] Side View (Static Camera)\n";
        std::cout << "  [4] Free Fly Camera (WASD to move, Q/E to fly up/down, Mouse to look)\n";
        std::cout << "Press [SPACE] to pause/resume simulation, and [+/-] to speed up/slow down.\n";
        std::cout << "Performance toggles: [F2] VSync, [G] Bloom, [F3] Bloom passes, [F4] Asteroids, [F5] Orbits, [F6] Text, [F7] Sphere LOD, [F8] Debug logging.\n";
        std::cout << "Press [ESC] inside the window to exit.\n";

        double lastTime = glfwGetTime();
        double lastFPSTime = lastTime;
        int frameCount = 0;
        float smoothedFrameTimeMs = 1000.0f / 60.0f;
        float displayFPS = 60.0f;
        float displayFrameMs = 1000.0f / 60.0f;

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

        bool bloomEnabled = true;
        bool gWasPressed = false;
        bool reducedBloomPasses = true;
        bool f2WasPressed = false;
        bool f3WasPressed = false;
        bool f4WasPressed = false;
        bool f5WasPressed = false;
        bool f6WasPressed = false;
        bool f7WasPressed = false;
        bool f8WasPressed = false;
        bool textOverlayEnabled = true;
        bool debugLoggingEnabled = false;
        OrbitQuality orbitQuality = OrbitQuality::High;
        bool vignetteActive = true;
        bool vWasPressed = false;

        DemoStage currentDemo = DemoStage::NONE;
        float demoTimer = 0.0f;
        bool f1WasPressed = false;
        bool demoPaused = false;
        bool letterboxActive = true;
        bool lWasPressed = false;


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
            std::string concept;
            std::string highlight;
            float speed;
            glm::vec3 pos0, pos1, pos2, pos3;
            glm::vec3 look0, look1, look2, look3;
        };

        std::vector<DemoSubStage> subStages = {
            // 1. Opening Wide (0-15s)
            {
                DemoStage::OPENING_WIDE, "Opening Wide", 0.0f, 15.0f,
                "Realistic Solar System Explorer",
                "An interactive OpenGL simulation of our celestial neighborhood, showcasing orbits, textures, lighting, and spacecraft.",
                "Perspective Projection, Camera Transforms & Orbital Trajectories", "", 0.8f,
                glm::vec3(16.0f, 8.0f, -16.0f), glm::vec3(20.0f, 10.0f, -20.0f), glm::vec3(-24.0f, 13.0f, 24.0f), glm::vec3(-30.0f, 15.0f, 30.0f),
                glm::vec3(0.0f), glm::vec3(0.0f), glm::vec3(0.0f), glm::vec3(0.0f)
            },
            // 2. Sun Zoom (15-35s)
            {
                DemoStage::SUN_ZOOM, "Sun Zoom", 15.0f, 20.0f,
                "The Sun: Central Star",
                "The central star of our system. Animates with a procedural halo glow and serves as the primary point light source.",
                "Procedural Texture Animation & Point Light Emission", "Sun", 1.0f,
                glm::vec3(5.5f, 1.8f, -1.0f), glm::vec3(5.0f, 1.5f, 0.0f), glm::vec3(-2.8f, 0.5f, 2.2f), glm::vec3(-4.0f, -0.2f, 3.0f),
                glm::vec3(0.0f), glm::vec3(0.0f), glm::vec3(0.05f, 0.0f, -0.05f), glm::vec3(0.0f)
            },
            // 3. Mercury View (35-41s)
            {
                DemoStage::INNER_PLANETS, "Mercury Fly-By", 35.0f, 6.0f,
                "Mercury: Scorched World",
                "The smallest and closest planet to the Sun, orbiting at high speed in a scorched, cratered environment.",
                "Keplerian Orbital Dynamics & Dynamic Translation", "Mercury", 3.5f,
                glm::vec3(0.38f, 0.14f, -0.15f), glm::vec3(0.34f, 0.12f, 0.0f), glm::vec3(-0.22f, 0.06f, 0.16f), glm::vec3(-0.28f, -0.02f, 0.22f),
                glm::vec3(0.0f), glm::vec3(0.0f), glm::vec3(0.0f), glm::vec3(0.0f)
            },
            // 4. Venus View (41-47s)
            {
                DemoStage::INNER_PLANETS, "Venus Fly-By", 41.0f, 6.0f,
                "Venus: Greenhouse Hell",
                "Earth's sister planet, shrouded in a dense, highly reflective carbon dioxide atmosphere with extreme surface heat.",
                "Diffuse Texture Mapping & Ambient/Specular Phong Shading", "Venus", 2.5f,
                glm::vec3(0.52f, 0.18f, -0.25f), glm::vec3(0.45f, 0.15f, 0.0f), glm::vec3(-0.32f, 0.08f, 0.25f), glm::vec3(-0.40f, -0.04f, 0.32f),
                glm::vec3(0.0f), glm::vec3(0.0f), glm::vec3(0.0f), glm::vec3(0.0f)
            },
            // 5. Earth View (47-53s)
            {
                DemoStage::INNER_PLANETS, "Earth Fly-By", 47.0f, 6.0f,
                "Earth: Blue Oasis",
                "Our home planet. The only known world in the universe hosting liquid surface water, an active biosphere, and human life.",
                "Multi-texturing, Atmosphere Scattering Glow & Alpha Blended Clouds", "Earth", 2.0f,
                glm::vec3(0.95f, 0.42f, -0.56f), glm::vec3(0.78f, 0.32f, 0.0f), glm::vec3(-0.58f, 0.18f, 0.42f), glm::vec3(-0.68f, -0.04f, 0.52f),
                glm::vec3(0.0f), glm::vec3(0.0f), glm::vec3(0.0f), glm::vec3(0.0f)
            },
            // 6. Mars View (53-60s)
            {
                DemoStage::INNER_PLANETS, "Mars Fly-By", 53.0f, 7.0f,
                "Mars: The Red Planet",
                "Featuring iron-rich rusty soil, thin atmosphere, polar carbon dioxide ice caps, and ancient volcanic valleys.",
                "Realistic Scale Factors & Specular Mapping", "Mars", 1.8f,
                glm::vec3(0.78f, 0.28f, -0.36f), glm::vec3(0.62f, 0.20f, 0.0f), glm::vec3(-0.46f, 0.10f, 0.34f), glm::vec3(-0.56f, -0.04f, 0.42f),
                glm::vec3(0.0f), glm::vec3(0.0f), glm::vec3(0.0f), glm::vec3(0.0f)
            },
            // 7. Earth & Moon (60-80s)
            {
                DemoStage::EARTH_MOON, "Earth & Moon System", 60.0f, 20.0f,
                "Earth & Moon System",
                "Observing the Earth and its tidally locked natural satellite, illustrating scaling and relative orbital orbits.",
                "Relative Transformations & Dynamic Night Lights", "Earth", 1.5f,
                glm::vec3(1.8f, 0.7f, -1.5f), glm::vec3(1.2f, 0.4f, -0.8f), glm::vec3(-0.8f, 0.25f, 0.9f), glm::vec3(-1.2f, 0.15f, 1.5f),
                glm::vec3(0.0f), glm::vec3(0.0f), glm::vec3(0.0f), glm::vec3(0.0f)
            },
            // 8. Space Station Orbit (80-100s)
            {
                DemoStage::SPACE_STATION, "ISS Close Fly-By", 80.0f, 20.0f,
                "International Space Station",
                "A modular research laboratory in low Earth orbit, assembled using hierarchical truss lines and solar panels.",
                "Hierarchical Model Assemblies & Local Euler Rotations", "SpaceStation", 2.5f,
                glm::vec3(0.25f, 0.08f, 0.0f), glm::vec3(0.0f, 0.15f, 0.25f), glm::vec3(-0.25f, 0.08f, 0.0f), glm::vec3(0.0f, 0.04f, -0.28f),
                glm::vec3(0.0f), glm::vec3(0.0f), glm::vec3(0.0f), glm::vec3(0.0f)
            },
            // 9. Spacecraft Mission (100-125s)
            {
                DemoStage::SPACECRAFT, "Spacecraft Mission", 100.0f, 25.0f,
                "Interplanetary Spacecraft",
                "An explorer spacecraft performing a Hohmann-inspired transfer trajectory from Earth to Mars with glowing exhaust.",
                "Tangent-based Trajectory Tracking & Follow Camera Math", "Spacecraft", 1.2f,
                glm::vec3(-0.05f, 0.03f, -0.12f), glm::vec3(-0.04f, 0.02f, -0.08f), glm::vec3(0.06f, 0.015f, 0.03f), glm::vec3(0.09f, 0.01f, 0.06f),
                glm::vec3(0.0f), glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, 0.05f), glm::vec3(0.0f, 0.0f, 0.10f)
            },
            // 10. Asteroid Belt (125-145s)
            {
                DemoStage::ASTEROID_BELT, "Asteroid Belt Navigation", 125.0f, 20.0f,
                "The Asteroid Belt",
                "Navigating through the annular debris belt situated between the orbits of Mars and Jupiter.",
                "Dynamic Instanced Particle Simulation & GL_POINTS; GL_POINTS instanced simulation, 800 varied particles, 3 size tiers", "AsteroidBelt", 1.5f,
                glm::vec3(7.2f, 1.0f, -0.7f), glm::vec3(7.8f, 0.4f, 1.2f), glm::vec3(8.8f, -0.2f, 2.8f), glm::vec3(9.6f, -0.5f, 4.0f),
                glm::vec3(0.0f), glm::vec3(0.0f), glm::vec3(0.0f), glm::vec3(0.0f)
            },
            // 11. Jupiter View (145-155s)
            {
                DemoStage::JUPITER_SATURN, "Jupiter Fly-By", 145.0f, 10.0f,
                "Jupiter: The Gas Giant",
                "The largest planet in the solar system, a massive gas giant with prominent atmospheric bands and multiple moons.",
                "Scale-Proportional Texture Coordinates & Orbit Tracing", "Jupiter", 0.9f,
                glm::vec3(2.2f, 0.9f, -1.4f), glm::vec3(1.9f, 0.6f, 0.0f), glm::vec3(-1.5f, 0.35f, 1.5f), glm::vec3(-1.8f, 0.15f, 2.0f),
                glm::vec3(0.0f), glm::vec3(0.0f), glm::vec3(0.0f), glm::vec3(0.0f)
            },
            // 12. Saturn Rings View (155-165s)
            {
                DemoStage::JUPITER_SATURN, "Saturn Fly-By", 155.0f, 10.0f,
                "Saturn: Ring World",
                "Sweeping down close to Saturn's spectacular rings, composed of billions of water ice particles and dust.",
                "Custom Ring Mesh Geometry & Double-Sided Alpha Blending", "Saturn", 0.8f,
                glm::vec3(2.8f, 1.2f, -1.4f), glm::vec3(2.3f, 0.9f, 0.0f), glm::vec3(-1.9f, 0.05f, 1.7f), glm::vec3(-2.3f, -0.35f, 2.2f),
                glm::vec3(0.0f), glm::vec3(0.0f), glm::vec3(0.0f), glm::vec3(0.0f)
            },
            // 13. Credits/Final Wide (165-180s)
            {
                DemoStage::CREDITS, "Solar System Credits", 165.0f, 15.0f,
                "Solar System Simulation",
                "Built using C++, Modern OpenGL, GLFW, Glad, and GLM. Thank you for watching!",
                "Post-Processing Framebuffers, HDR Tone Mapping & Bloom", "", 0.5f,
                glm::vec3(0.0f, 8.0f, 24.0f), glm::vec3(0.0f, 10.0f, 28.0f), glm::vec3(0.0f, 22.0f, 42.0f), glm::vec3(0.0f, 28.0f, 52.0f),
                glm::vec3(0.0f), glm::vec3(0.0f), glm::vec3(0.0f), glm::vec3(0.0f)
            }
        };


        auto getSubStageIdealState = [&](int idx, float t, glm::vec3& pos, glm::vec3& lookAt) {
            if (idx < 0 || idx >= static_cast<int>(subStages.size())) {
                pos = glm::vec3(0.0f, 10.0f, 15.0f);
                lookAt = glm::vec3(0.0f);
                return;
            }

            const auto& sub = subStages[idx];
            float stageTime = t - sub.startTime;
            float u = stageTime / sub.duration;
            u = glm::clamp(u, 0.0f, 1.0f);

            // Interpolate relative camera position and look-at using Catmull-Rom
            glm::vec3 relPos = CameraHelpers::interpolateCatmullRom(sub.pos0, sub.pos1, sub.pos2, sub.pos3, u);
            glm::vec3 relLook = CameraHelpers::interpolateCatmullRom(sub.look0, sub.look1, sub.look2, sub.look3, u);

            // Determine focused body position and orientation
            glm::vec3 focusPos(0.0f);
            bool useLocalFrame = false;
            glm::vec3 localRight(1.0f, 0.0f, 0.0f);
            glm::vec3 localUp(0.0f, 1.0f, 0.0f);
            glm::vec3 localFwd(0.0f, 0.0f, -1.0f);

            if (sub.highlight == "Sun") {
                focusPos = sun ? sun->getPosition() : glm::vec3(0.0f);
            } else if (sub.highlight == "Mercury") {
                focusPos = mercury ? mercury->getPosition() : glm::vec3(0.0f);
            } else if (sub.highlight == "Venus") {
                focusPos = venus ? venus->getPosition() : glm::vec3(0.0f);
            } else if (sub.highlight == "Earth") {
                focusPos = earth ? earth->getPosition() : glm::vec3(0.0f);
            } else if (sub.highlight == "Mars") {
                focusPos = mars ? mars->getPosition() : glm::vec3(0.0f);
            } else if (sub.highlight == "Jupiter") {
                focusPos = jupiter ? jupiter->getPosition() : glm::vec3(0.0f);
            } else if (sub.highlight == "Saturn") {
                focusPos = saturn ? saturn->getPosition() : glm::vec3(0.0f);
            } else if (sub.highlight == "SpaceStation" && spaceStation) {
                focusPos = spaceStation->getPosition();
                glm::mat4 model = spaceStation->getTransform().getModelMatrix();
                localRight = glm::normalize(glm::vec3(model[0]));
                localUp = glm::normalize(glm::vec3(model[1]));
                localFwd = glm::normalize(glm::vec3(model[2]));
                useLocalFrame = true;
            } else if (sub.highlight == "Spacecraft" && spacecraft) {
                focusPos = spacecraft->getPosition();
                localFwd = spacecraft->getForwardDir();
                glm::vec3 worldUp = glm::vec3(0.0f, 1.0f, 0.0f);
                localRight = glm::normalize(glm::cross(localFwd, worldUp));
                localUp = glm::normalize(glm::cross(localRight, localFwd));
                useLocalFrame = true;
            } else if (sub.highlight == "AsteroidBelt") {
                focusPos = glm::vec3(0.0f); // centered around origin
            }

            if (useLocalFrame) {
                pos = focusPos + relPos.x * localRight + relPos.y * localUp + relPos.z * localFwd;
                lookAt = focusPos + relLook.x * localRight + relLook.y * localUp + relLook.z * localFwd;
            } else {
                pos = focusPos + relPos;
                lookAt = focusPos + relLook;
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
        auto renderCenteredText = [&](const std::string& text, float centerY, float scale, const glm::vec4& color, int maxChars) {
            std::vector<std::string> lines;
            std::string currentLine = "";
            std::stringstream ss(text);
            std::string word;
            while (ss >> word) {
                if (currentLine.length() + word.length() + 1 > static_cast<size_t>(maxChars)) {
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

            float lineSpacing = 18.0f * scale;
            float totalHeight = lines.size() * lineSpacing;
            float startY = centerY + (totalHeight / 2.0f) - (lineSpacing / 2.0f);
            
            int screenW = window.getWidth();
            int screenH = window.getHeight();

            for (size_t i = 0; i < lines.size(); ++i) {
                float lineWidth = lines[i].length() * 8.0f * scale;
                float lineX = (static_cast<float>(screenW) - lineWidth) / 2.0f;
                float lineY = startY - i * lineSpacing;
                
                // Shadow
                textRenderer.renderText(lines[i], lineX + 1.0f, lineY - 1.0f, scale, glm::vec4(0.0f, 0.0f, 0.0f, color.a * 0.8f), screenW, screenH);
                // Main text
                textRenderer.renderText(lines[i], lineX, lineY, scale, color, screenW, screenH);
            }
        };

        // Main render loop
        while (!window.shouldClose()) {
            double currentTime = glfwGetTime();
            float rawDeltaTime = static_cast<float>(currentTime - lastTime);
            lastTime = currentTime;
            if (rawDeltaTime < 0.0f) {
                rawDeltaTime = 0.0f;
            }
            const float deltaTime = glm::clamp(rawDeltaTime, 0.0f, 1.0f / 20.0f);
            smoothedFrameTimeMs = glm::mix(smoothedFrameTimeMs, deltaTime * 1000.0f, 0.08f);
            displayFrameMs = smoothedFrameTimeMs;
            displayFPS = displayFrameMs > 0.001f ? 1000.0f / displayFrameMs : 0.0f;

            if (debugLoggingEnabled && rawDeltaTime > 0.12f) {
                std::cout << "Frame spike: raw=" << (rawDeltaTime * 1000.0f)
                          << "ms clamped=" << (deltaTime * 1000.0f) << "ms\n";
            }

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
                    sysCamera->setTargetView(glm::vec3(0.0f, 9.5f, 18.0f), glm::vec3(0.0f, 0.0f, 0.0f));
                } else if (glfwGetKey(glWin, GLFW_KEY_2) == GLFW_PRESS) {
                    sceneManager.setActiveCamera(sysCamera);
                    sysCamera->setTargetView(glm::vec3(0.0f, 30.0f, 0.8f), glm::vec3(0.0f, 0.0f, 0.0f));
                } else if (glfwGetKey(glWin, GLFW_KEY_3) == GLFW_PRESS) {
                    sceneManager.setActiveCamera(sysCamera);
                    sysCamera->setTargetView(glm::vec3(28.0f, 1.5f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f));
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

            // G key to toggle cinematic bloom
            bool gIsPressed = (glfwGetKey(glWin, GLFW_KEY_G) == GLFW_PRESS);
            if (gIsPressed && !gWasPressed) {
                bloomEnabled = !bloomEnabled;
                std::cout << "Cinematic Bloom " << (bloomEnabled ? "ENABLED" : "DISABLED") << std::endl;
            }
            gWasPressed = gIsPressed;

            bool f2IsPressed = (glfwGetKey(glWin, GLFW_KEY_F2) == GLFW_PRESS);
            if (f2IsPressed && !f2WasPressed) {
                window.setVSync(!window.isVSyncEnabled());
                std::cout << "VSync " << (window.isVSyncEnabled() ? "ENABLED" : "DISABLED") << std::endl;
            }
            f2WasPressed = f2IsPressed;

            bool f3IsPressed = (glfwGetKey(glWin, GLFW_KEY_F3) == GLFW_PRESS);
            if (f3IsPressed && !f3WasPressed) {
                reducedBloomPasses = !reducedBloomPasses;
                renderer.setBloomBlurPasses(reducedBloomPasses ? 4u : 6u);
                std::cout << "Bloom blur passes: " << renderer.getBloomBlurPasses() << std::endl;
            }
            f3WasPressed = f3IsPressed;

            bool f4IsPressed = (glfwGetKey(glWin, GLFW_KEY_F4) == GLFW_PRESS);
            if (f4IsPressed && !f4WasPressed) {
                AsteroidBelt::Quality nextQuality = AsteroidBelt::Quality::Medium;
                if (asteroidBelt->getQuality() == AsteroidBelt::Quality::Medium) {
                    nextQuality = AsteroidBelt::Quality::High;
                } else if (asteroidBelt->getQuality() == AsteroidBelt::Quality::High) {
                    nextQuality = AsteroidBelt::Quality::Low;
                }
                asteroidBelt->setQuality(nextQuality);
                std::cout << "Asteroid quality: " << asteroidQualityName(nextQuality) << std::endl;
            }
            f4WasPressed = f4IsPressed;

            bool f5IsPressed = (glfwGetKey(glWin, GLFW_KEY_F5) == GLFW_PRESS);
            if (f5IsPressed && !f5WasPressed) {
                orbitQuality = (orbitQuality == OrbitQuality::High) ? OrbitQuality::Low : OrbitQuality::High;
                const bool highQualityOrbits = orbitQuality == OrbitQuality::High;
                for (const auto& orbit : allOrbits) {
                    if (orbit) {
                        orbit->setHighQuality(highQualityOrbits);
                    }
                }
                std::cout << "Orbit quality: " << (highQualityOrbits ? "HIGH" : "LOW") << std::endl;
            }
            f5WasPressed = f5IsPressed;

            bool f6IsPressed = (glfwGetKey(glWin, GLFW_KEY_F6) == GLFW_PRESS);
            if (f6IsPressed && !f6WasPressed) {
                textOverlayEnabled = !textOverlayEnabled;
                std::cout << "Text overlay " << (textOverlayEnabled ? "ENABLED" : "DISABLED") << std::endl;
            }
            f6WasPressed = f6IsPressed;

            bool f7IsPressed = (glfwGetKey(glWin, GLFW_KEY_F7) == GLFW_PRESS);
            if (f7IsPressed && !f7WasPressed) {
                renderer.setSphereMeshHighQuality(!renderer.isSphereMeshHighQuality());
                std::cout << "Sphere mesh quality: " << (renderer.isSphereMeshHighQuality() ? "HIGH" : "LOW") << std::endl;
            }
            f7WasPressed = f7IsPressed;

            bool f8IsPressed = (glfwGetKey(glWin, GLFW_KEY_F8) == GLFW_PRESS);
            if (f8IsPressed && !f8WasPressed) {
                debugLoggingEnabled = !debugLoggingEnabled;
                std::cout << "Debug frame logging " << (debugLoggingEnabled ? "ENABLED" : "DISABLED") << std::endl;
            }
            f8WasPressed = f8IsPressed;

            if (bloomEnabled && displayFPS < 45.0f && renderer.getBloomBlurPasses() > 2u) {
                renderer.setBloomBlurPasses(2u);
                reducedBloomPasses = true;
                if (debugLoggingEnabled) {
                    std::cout << "Auto-reduced bloom blur passes to 2 after low FPS.\n";
                }
            }

            // L key to toggle letterbox
            bool lIsPressed = (glfwGetKey(glWin, GLFW_KEY_L) == GLFW_PRESS);
            if (lIsPressed && !lWasPressed) {
                letterboxActive = !letterboxActive;
            }
            lWasPressed = lIsPressed;

            // V key to toggle vignette post-effect
            bool vIsPressed = (glfwGetKey(glWin, GLFW_KEY_V) == GLFW_PRESS);
            if (vIsPressed && !vWasPressed) {
                vignetteActive = !vignetteActive;
                std::cout << "Vignette Effect " << (vignetteActive ? "ENABLED" : "DISABLED") << std::endl;
            }
            vWasPressed = vIsPressed;


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

            // Clear screen & bind HDR Framebuffer
            renderer.beginFrame();

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
                    glm::vec3 finalFront = glm::vec3(0.0f, 0.0f, -1.0f);
                    bool hasFinalFront = false;
                    if (glm::length(idealLookAt - idealPos) > 0.0001f) {
                        finalFront = glm::normalize(idealLookAt - idealPos);
                        hasFinalFront = true;
                    }

                    const float transitionDuration = 3.5f;
                    if (stageTime < transitionDuration) {
                        if (currentSubStageIdx > 0) {
                            float prevEndTime = subStages[currentSubStageIdx - 1].startTime + subStages[currentSubStageIdx - 1].duration;
                            glm::vec3 prevPos, prevLookAt;
                            getSubStageIdealState(currentSubStageIdx - 1, prevEndTime, prevPos, prevLookAt);

                            float u = stageTime / transitionDuration;
                            float s = u * u * u * (u * (u * 6.0f - 15.0f) + 10.0f);
                            finalPos = glm::mix(prevPos, idealPos, s);
                            glm::vec3 fromDir = prevLookAt - prevPos;
                            glm::vec3 toDir = idealLookAt - idealPos;
                            if (glm::length(fromDir) > 0.0001f && glm::length(toDir) > 0.0001f) {
                                glm::quat qFrom = glm::quatLookAt(glm::normalize(fromDir), glm::vec3(0.0f, 1.0f, 0.0f));
                                glm::quat qTo = glm::quatLookAt(glm::normalize(toDir), glm::vec3(0.0f, 1.0f, 0.0f));
                                glm::quat qBlend = glm::slerp(qFrom, qTo, s);
                                finalFront = glm::normalize(qBlend * glm::vec3(0.0f, 0.0f, -1.0f));
                                hasFinalFront = true;
                            }
                        } else {
                            float u = stageTime / transitionDuration;
                            float s = u * u * u * (u * (u * 6.0f - 15.0f) + 10.0f);
                            glm::vec3 prevPos = demoStartCamPos;
                            glm::vec3 prevLookAt = demoStartCamPos + demoStartCamFront * 5.0f;
                            finalPos = glm::mix(prevPos, idealPos, s);
                            glm::vec3 fromDir = prevLookAt - prevPos;
                            glm::vec3 toDir = idealLookAt - idealPos;
                            if (glm::length(fromDir) > 0.0001f && glm::length(toDir) > 0.0001f) {
                                glm::quat qFrom = glm::quatLookAt(glm::normalize(fromDir), glm::vec3(0.0f, 1.0f, 0.0f));
                                glm::quat qTo = glm::quatLookAt(glm::normalize(toDir), glm::vec3(0.0f, 1.0f, 0.0f));
                                glm::quat qBlend = glm::slerp(qFrom, qTo, s);
                                finalFront = glm::normalize(qBlend * glm::vec3(0.0f, 0.0f, -1.0f));
                                hasFinalFront = true;
                            }
                        }
                    }

                    auto activeCam = sceneManager.getActiveCamera();
                    if (activeCam) {
                        activeCam->setPosition(finalPos);
                        if (hasFinalFront) {
                            activeCam->setFront(finalFront);
                        } else if (glm::length(finalLookAt - finalPos) > 0.0001f) {
                            activeCam->setFront(glm::normalize(finalLookAt - finalPos));
                        }
                        glm::vec3 worldUp = glm::vec3(0.0f, 1.0f, 0.0f);
                        glm::vec3 right = glm::normalize(glm::cross(activeCam->getFront(), worldUp));
                        activeCam->setUp(glm::normalize(glm::cross(right, activeCam->getFront())));
                    }
                }
            }

            for (std::size_t i = 0; i < solarOrbits.size(); ++i) {
                solarOrbits[i]->setEmphasized(static_cast<int>(i) == selectedPlanetIdx);
            }

            // Update scene objects
            float effectiveDelta = animationPaused ? 0.0f : (deltaTime * animationSpeed);
            sceneManager.update(effectiveDelta);

#ifndef NDEBUG
            static bool warnedMoonStationDynamicOverlap = false;
            static bool warnedIoEuropaDynamicOverlap = false;
            auto warnDynamicOverlapOnce = [](const std::string& aName,
                                             const glm::vec3& aPos,
                                             float aRadius,
                                             const std::string& bName,
                                             const glm::vec3& bPos,
                                             float bRadius,
                                             float margin,
                                             bool& warned) {
                if (!warned && wouldOverlap(aPos, aRadius, bPos, bRadius, margin)) {
                    warnLayout(aName + " updated too close to " + bName + ".");
                    warned = true;
                }
            };
            warnDynamicOverlapOnce("Moon", moon->getPosition(), moon->getRadius(),
                                   "SpaceStation", spaceStation->getPosition(), kSpaceStationBoundsRadius,
                                   0.06f, warnedMoonStationDynamicOverlap);
            warnDynamicOverlapOnce("Io", io->getPosition(), io->getRadius(),
                                   "Europa", europa->getPosition(), europa->getRadius(),
                                   0.08f, warnedIoEuropaDynamicOverlap);
#endif

            // Render scene
            sceneManager.render(renderer);

            // End FBO rendering and apply post-processing
            float vignetteStrength = (currentDemo != DemoStage::NONE && vignetteActive) ? 1.0f : 0.0f;
            renderer.endFrame(bloomEnabled, vignetteStrength);

            // Render text overlays (instructions in corners)
            int width = window.getWidth();
            int height = window.getHeight();
            // Setup common styles/colors
            glm::vec4 manualBorderCol(0.15f, 0.30f, 0.55f, 0.40f);
            glm::vec4 manualBgCol(0.06f, 0.08f, 0.12f, 0.75f);

            if (textOverlayEnabled) {
            // 1. --- FLOATING PLANET NAME LABELS (Manual mode only) ---
            if (currentDemo == DemoStage::NONE && sceneManager.getActiveCamera()) {
                float aspect = static_cast<float>(width) / static_cast<float>(height);
                glm::mat4 view = sceneManager.getActiveCamera()->getViewMatrix();
                glm::mat4 projection = sceneManager.getActiveCamera()->getProjectionMatrix(aspect);
                glm::vec3 camPos = sceneManager.getActiveCamera()->getPosition();
                
                struct LabelTarget {
                    std::string name;
                    glm::vec3 pos;
                    float radius;
                    glm::vec3 color;
                };
                std::vector<LabelTarget> labelTargets;
                labelTargets.push_back({ "SUN", sun->getPosition(), sun->getTransform().getScale().x, glm::vec3(1.0f, 0.75f, 0.2f) });
                for (const auto& p : planets) {
                    std::string nameUpper = p->getName();
                    std::transform(nameUpper.begin(), nameUpper.end(), nameUpper.begin(), ::toupper);
                    labelTargets.push_back({ nameUpper, p->getPosition(), p->getRadius(), glm::vec3(0.8f, 0.9f, 1.0f) });
                }
                if (spacecraft) {
                    labelTargets.push_back({ "SPACECRAFT", spacecraft->getPosition(), 0.05f, glm::vec3(0.0f, 1.0f, 0.8f) });
                }
                if (spaceStation) {
                    labelTargets.push_back({ "SPACE STATION", spaceStation->getPosition(), 0.05f, glm::vec3(0.0f, 1.0f, 0.8f) });
                }
                
                for (const auto& target : labelTargets) {
                    float dist = glm::distance(camPos, target.pos);
                    
                    // Fade out labels if camera gets too close to prevent overlapping
                    float minFadeDist = target.radius * 2.0f;
                    float maxFadeDist = target.radius * 4.0f;
                    float labelAlpha = 1.0f;
                    if (dist < minFadeDist) {
                        labelAlpha = 0.0f;
                    } else if (dist < maxFadeDist) {
                        labelAlpha = (dist - minFadeDist) / (maxFadeDist - minFadeDist);
                    }
                    
                    if (labelAlpha > 0.0f) {
                        glm::vec2 screenPos;
                        if (projectWorldToScreen(target.pos, view, projection, width, height, screenPos)) {
                            float labelScale = 0.9f * UI_SCALE;
                            float textW = target.name.length() * 8.0f * labelScale;
                            float labelX = screenPos.x - (textW / 2.0f);
                            float labelY = screenPos.y + 12.0f * UI_SCALE;
                            
                            textRenderer.renderText(target.name, labelX, labelY, labelScale, glm::vec4(target.color, labelAlpha * 0.85f), width, height);
                        }
                    }
                }
            }

            // 2. --- CINEMATIC DEMO MODE OVERLAY (F1 Demo only) ---
            if (currentDemo != DemoStage::NONE) {
                float stageTime = demoTimer - subStages[currentSubStageIdx].startTime;
                float duration = subStages[currentSubStageIdx].duration;
                
                // A. Render Letterbox Black Bars (on top of 3D, behind HUD text)
                float barHeight = height * 0.12f;
                if (letterboxActive) {
                    // Bottom bar
                    textRenderer.renderQuad(0.0f, 0.0f, static_cast<float>(width), barHeight, glm::vec4(0.0f, 0.0f, 0.0f, 1.0f), width, height);
                    // Top bar
                    textRenderer.renderQuad(0.0f, static_cast<float>(height) - barHeight, static_cast<float>(width), barHeight, glm::vec4(0.0f, 0.0f, 0.0f, 1.0f), width, height);
                }

                // Demo progress bar:
                float barW = width * 0.70f;
                float barX = (width - barW) / 2.0f;
                float fill = demoTimer / 180.0f;
                // Background track
                textRenderer.renderQuad(barX, 8.0f, barW, 4.0f,
                                        glm::vec4(1.0f, 1.0f, 1.0f, 0.12f), width, height);
                // Gold fill
                textRenderer.renderQuad(barX, 8.0f, barW * fill, 4.0f,
                                        glm::vec4(0.85f, 0.68f, 0.15f, 0.85f), width, height);
                // Playhead dot
                textRenderer.renderQuad(barX + barW * fill - 2.0f, 6.0f, 4.0f, 8.0f,
                                        glm::vec4(1.0f, 1.0f, 1.0f, 1.0f), width, height);

                // Stage count and current scene name: "Scene X of Y - Name"
                {
                    std::string stageText = "Scene " + std::to_string(currentSubStageIdx + 1) + " of " + std::to_string(subStages.size()) + " - " + subStages[currentSubStageIdx].name;
                    float stageScale = 0.7f * UI_SCALE;
                    float stageW = stageText.length() * 8.0f * stageScale;
                    float stageX = (width - stageW) / 2.0f;
                    float stageY = 1.0f;
                    textRenderer.renderText(stageText, stageX + 1.0f, stageY - 1.0f, stageScale, glm::vec4(0.0f, 0.0f, 0.0f, 0.7f), width, height);
                    textRenderer.renderText(stageText, stageX, stageY, stageScale, glm::vec4(0.6f, 0.6f, 0.6f, 0.8f), width, height);
                }


                // B. Fade-in/fade-out logic for the HUD overlay (1.5s fade at ends)
                float hudAlpha = 1.0f;
                if (stageTime < 1.5f) {
                    hudAlpha = stageTime / 1.5f;
                } else if (stageTime > duration - 1.5f) {
                    hudAlpha = (duration - stageTime) / 1.5f;
                }
                hudAlpha = glm::clamp(hudAlpha, 0.0f, 1.0f);

                // C. Floating panel for bottom HUD if letterbox is not active (to ensure readability)
                if (hudAlpha > 0.0f) {
                    if (!letterboxActive) {
                        float panelW = width * 0.75f;
                        float panelH = barHeight * 0.9f;
                        float panelX = (width - panelW) / 2.0f;
                        float panelY = 10.0f;
                        glm::vec4 panelBg = glm::vec4(0.06f, 0.08f, 0.12f, 0.75f * hudAlpha);
                        glm::vec4 panelBorder = glm::vec4(0.2f, 1.0f, 0.6f, 0.3f * hudAlpha);
                        textRenderer.renderQuad(panelX - 1.0f, panelY - 1.0f, panelW + 2.0f, panelH + 2.0f, panelBorder, width, height);
                        textRenderer.renderQuad(panelX, panelY, panelW, panelH, panelBg, width, height);
                    }

                    // D. Draw bottom HUD text
                    const auto& sub = subStages[currentSubStageIdx];
                    
                    // Center heights relative to bottom area
                    float bottomCenterY = letterboxActive ? (barHeight * 0.5f) : (10.0f + (barHeight * 0.9f) * 0.5f);
                    
                    float titleY = bottomCenterY + 22.0f * UI_SCALE;
                    float expY = bottomCenterY;
                    float conceptY = bottomCenterY - 20.0f * UI_SCALE;

                    // 1. Scene Title
                    glm::vec4 titleCol = glm::vec4(1.0f, 0.85f, 0.3f, hudAlpha);
                    std::string sceneTitleStr = sub.name;
                    std::transform(sceneTitleStr.begin(), sceneTitleStr.end(), sceneTitleStr.begin(), ::toupper);
                    float titleScale = 1.3f * UI_SCALE;
                    float titleW = sceneTitleStr.length() * 8.0f * titleScale;
                    float titleX = (width - titleW) / 2.0f;
                    textRenderer.renderText(sceneTitleStr, titleX + 1.0f, titleY - 1.0f, titleScale, glm::vec4(0.0f, 0.0f, 0.0f, hudAlpha * 0.8f), width, height);
                    textRenderer.renderText(sceneTitleStr, titleX, titleY, titleScale, titleCol, width, height);

                    // 2. Explanation (Wrapped)
                    glm::vec4 expCol = glm::vec4(0.9f, 0.9f, 0.9f, hudAlpha);
                    int maxChars = letterboxActive ? (width - 60) / (8.0f * UI_SCALE) : (width * 0.7f) / (8.0f * UI_SCALE);
                    renderCenteredText(sub.explanation, expY, 1.0f * UI_SCALE, expCol, maxChars);

                    // 3. CG Concept Demonstrated
                    glm::vec4 conceptCol = glm::vec4(0.2f, 1.0f, 0.6f, hudAlpha);
                    std::string conceptStr = "CG CONCEPT: " + sub.concept;
                    float conceptScale = 0.85f * UI_SCALE;
                    float conceptW = conceptStr.length() * 8.0f * conceptScale;
                    float conceptX = (width - conceptW) / 2.0f;
                    textRenderer.renderText(conceptStr, conceptX + 1.0f, conceptY - 1.0f, conceptScale, glm::vec4(0.0f, 0.0f, 0.0f, hudAlpha * 0.8f), width, height);
                    textRenderer.renderText(conceptStr, conceptX, conceptY, conceptScale, conceptCol, width, height);
                }

                // E. Render Large Cinematic Center Titles (First 4s of each stage)
                float titleAlpha = 0.0f;
                float titleDisplayDuration = 4.0f;
                if (stageTime < titleDisplayDuration) {
                    if (stageTime < 1.0f) {
                        titleAlpha = stageTime / 1.0f;
                    } else if (stageTime > titleDisplayDuration - 1.0f) {
                        titleAlpha = (titleDisplayDuration - stageTime) / 1.0f;
                    } else {
                        titleAlpha = 1.0f;
                    }
                }
                titleAlpha = glm::clamp(titleAlpha, 0.0f, 1.0f);

                if (stageTime < 2.0f) {
                    float sceneAlpha = glm::clamp(stageTime / 2.0f, 0.0f, 1.0f);
                    float bgAlpha = 1.0f - sceneAlpha * sceneAlpha;
                    textRenderer.renderQuad(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height),
                                            glm::vec4(0.0f, 0.0f, 0.0f, bgAlpha * 0.88f), width, height);
                }

                if (titleAlpha > 0.0f) {
                    const auto& titleSub = subStages[currentSubStageIdx];
                    std::string largeTitleStr = titleSub.title;
                    std::transform(largeTitleStr.begin(), largeTitleStr.end(), largeTitleStr.begin(), ::toupper);
                    float scale = 2.4f * UI_SCALE;
                    float textW = largeTitleStr.length() * 8.0f * scale;
                    float textX = (width - textW) / 2.0f;
                    float textY = height * 0.55f;

                    if (titleSub.stage == DemoStage::CREDITS) {
                        textY += (demoTimer - 165.0f) * 12.0f;
                    }

                    // Shadow
                    textRenderer.renderText(largeTitleStr, textX + 2.0f, textY - 2.0f, scale, glm::vec4(0.0f, 0.0f, 0.0f, titleAlpha * 0.85f), width, height);
                    // Foreground
                    textRenderer.renderText(largeTitleStr, textX, textY, scale, glm::vec4(1.0f, 0.9f, 0.5f, titleAlpha), width, height);

                    textRenderer.renderQuad(textX - 20.0f, textY - 6.0f, textW + 40.0f, 3.0f,
                                            glm::vec4(0.85f, 0.68f, 0.15f, titleAlpha), width, height);

                    std::string subtitleStr;
                    if (titleSub.stage == DemoStage::CREDITS) {
                        subtitleStr = "MISSION COMPLETE";
                    } else if (titleSub.highlight.empty()) {
                        subtitleStr = "SOLAR SYSTEM MISSION";
                    } else {
                        subtitleStr = getPlanetInfo(titleSub.highlight).type;
                    }
                    std::transform(subtitleStr.begin(), subtitleStr.end(), subtitleStr.begin(), ::toupper);
                    float subtitleScale = 0.95f * UI_SCALE;
                    float subtitleW = subtitleStr.length() * 8.0f * subtitleScale;
                    float subtitleX = (width - subtitleW) / 2.0f;
                    float subtitleY = textY - 34.0f * UI_SCALE;
                    textRenderer.renderText(subtitleStr, subtitleX + 2.0f, subtitleY - 2.0f, subtitleScale, glm::vec4(0.0f, 0.0f, 0.0f, titleAlpha * 0.85f), width, height);
                    textRenderer.renderText(subtitleStr, subtitleX, subtitleY, subtitleScale, glm::vec4(0.78f, 0.82f, 0.88f, titleAlpha), width, height);
                }

                // F. Render Top-Right Demo Controls Reminder
                std::string reminderStr = "[SPACE] Pause  [F1] Restart  [ESC] Exit  [L] Letterbox: " + std::string(letterboxActive ? "ON" : "OFF") + "  [V] Vignette: " + std::string(vignetteActive ? "ON" : "OFF");
                float remScale = 0.85f * UI_SCALE;
                float remW = reminderStr.length() * 8.0f * remScale;
                float remX = width - remW - 20.0f;
                float remY = height - (letterboxActive ? (barHeight * 0.5f) : 25.0f);
                // Shadow
                textRenderer.renderText(reminderStr, remX + 1.0f, remY - 1.0f, remScale, glm::vec4(0.0f, 0.0f, 0.0f, 0.7f), width, height);
                // Foreground
                textRenderer.renderText(reminderStr, remX, remY, remScale, glm::vec4(0.7f, 0.85f, 1.0f, 0.8f), width, height);
            }


            // 3. --- TOP-LEFT PANEL (Simulation info, Manual mode only) ---
            if (currentDemo == DemoStage::NONE) {
                bool showSpacecraftTelemetry = (spacecraft != nullptr);
                float tlW = 380.0f * UI_SCALE;
                float tlH = (showSpacecraftTelemetry ? 120.0f : 60.0f) * UI_SCALE;
                float tlX = 15.0f;
                float tlY = height - tlH - 15.0f;
                
                textRenderer.renderQuad(tlX - 1.0f, tlY - 1.0f, tlW + 2.0f, tlH + 2.0f, manualBorderCol, width, height);
                textRenderer.renderQuad(tlX, tlY, tlW, tlH, manualBgCol, width, height);
                
                textRenderer.renderText("SOLAR SYSTEM SIMULATION", tlX + 15.0f * UI_SCALE, tlY + tlH - 25.0f * UI_SCALE, 1.6f * UI_SCALE, glm::vec3(1.0f, 0.85f, 0.3f), width, height);
                textRenderer.renderText("OpenGL | Computer Graphics", tlX + 15.0f * UI_SCALE, tlY + tlH - 45.0f * UI_SCALE, 1.0f * UI_SCALE, glm::vec3(0.7f, 0.75f, 0.8f), width, height);
                
                if (showSpacecraftTelemetry) {
                    // Draw separator line
                    textRenderer.renderQuad(tlX + 15.0f * UI_SCALE, tlY + tlH - 55.0f * UI_SCALE, tlW - 30.0f * UI_SCALE, 1.0f, manualBorderCol, width, height);
                    
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
                    
                    textRenderer.renderText("Mission: Earth to Mars", tlX + 15.0f * UI_SCALE, tlY + tlH - 75.0f * UI_SCALE, 1.0f * UI_SCALE, glm::vec3(0.0f, 0.9f, 1.0f), width, height);
                    textRenderer.renderText("Current target: Mars", tlX + 15.0f * UI_SCALE, tlY + tlH - 92.0f * UI_SCALE, 1.0f * UI_SCALE, glm::vec3(0.0f, 0.9f, 1.0f), width, height);
                    textRenderer.renderText(std::string("Progress: ") + progressPercentStr + bar, tlX + 15.0f * UI_SCALE, tlY + tlH - 109.0f * UI_SCALE, 1.0f * UI_SCALE, glm::vec3(0.0f, 0.9f, 1.0f), width, height);
                }
            }

            // 4. --- TOP-RIGHT PANEL (Camera & Speed, Manual mode only) ---
            if (currentDemo == DemoStage::NONE) {
                std::string camName = sceneManager.getActiveCamera() ? sceneManager.getActiveCamera()->getName() : "None";
                std::string camStr = "Camera: " + camName;
                
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
                
                char perfBuf[96];
                std::snprintf(perfBuf, sizeof(perfBuf), "FPS: %.0f | %.2f ms", displayFPS, displayFrameMs);

                float trW = 300.0f * UI_SCALE;
                float trH = 80.0f * UI_SCALE;
                float trX = width - trW - 15.0f;
                float trY = height - trH - 15.0f;
                
                textRenderer.renderQuad(trX - 1.0f, trY - 1.0f, trW + 2.0f, trH + 2.0f, manualBorderCol, width, height);
                textRenderer.renderQuad(trX, trY, trW, trH, manualBgCol, width, height);
                
                textRenderer.renderText(camStr, trX + 15.0f * UI_SCALE, trY + trH - 22.0f * UI_SCALE, 1.0f * UI_SCALE, glm::vec3(0.0f, 1.0f, 1.0f), width, height);
                textRenderer.renderText(speedOverlayStr, trX + 15.0f * UI_SCALE, trY + trH - 42.0f * UI_SCALE, 1.0f * UI_SCALE, speedColor, width, height);
                textRenderer.renderText(perfBuf, trX + 15.0f * UI_SCALE, trY + trH - 62.0f * UI_SCALE, 0.95f * UI_SCALE, glm::vec3(0.78f, 0.86f, 0.92f), width, height);
            }

            // 5. --- BOTTOM-LEFT PANEL (Controls, Manual mode only) ---
            if (currentDemo == DemoStage::NONE) {
                float blW = 500.0f * UI_SCALE;
                float blH = 75.0f * UI_SCALE;
                float blX = 15.0f;
                float blY = 15.0f;
                
                textRenderer.renderQuad(blX - 1.0f, blY - 1.0f, blW + 2.0f, blH + 2.0f, manualBorderCol, width, height);
                textRenderer.renderQuad(blX, blY, blW, blH, manualBgCol, width, height);
                
                textRenderer.renderText("[1-5] Cameras | [N/B] Next/Prev Planet | [ENTER] Focus", blX + 15.0f * UI_SCALE, blY + blH - 22.0f * UI_SCALE, 0.95f * UI_SCALE, glm::vec3(1.0f, 1.0f, 1.0f), width, height);
                textRenderer.renderText("[SPACE] Pause | [+/-] Speed | [F1] Demo | [G] Bloom: " + std::string(bloomEnabled ? "ON" : "OFF") + " | [F2-F8] Perf", blX + 15.0f * UI_SCALE, blY + blH - 42.0f * UI_SCALE, 0.95f * UI_SCALE, glm::vec3(1.0f, 1.0f, 1.0f), width, height);
                textRenderer.renderText("[ESC] Exit", blX + 15.0f * UI_SCALE, blY + blH - 62.0f * UI_SCALE, 0.95f * UI_SCALE, glm::vec3(0.5f, 0.5f, 0.5f), width, height);
            }

            // 6. --- RIGHT-SIDE INFORMATION OVERLAY HUD PANEL (Manual mode only) ---
            if (currentDemo == DemoStage::NONE) {
                bool showHudPanel = false;
                PlanetInfo hudInfo;
                if (selectedPlanetIdx >= 0 && selectedPlanetIdx < static_cast<int>(planets.size())) {
                    hudInfo = getPlanetInfo(planets[selectedPlanetIdx]->getName());
                    showHudPanel = true;
                }

                if (showHudPanel) {
                    float panelW = 320.0f * UI_SCALE;
                    float panelH = 430.0f * UI_SCALE;
                    float panelX = width - panelW - 15.0f;
                    float panelY = height - panelH - 90.0f * UI_SCALE; // fits nicely below top-right camera/speed text

                    glm::vec4 borderColor = glm::vec4(0.0f, 0.8f, 1.0f, 0.4f);
                    textRenderer.renderQuad(panelX - 1.0f, panelY - 1.0f, panelW + 2.0f, panelH + 2.0f, borderColor, width, height);
                    textRenderer.renderQuad(panelX, panelY, panelW, panelH, manualBgCol, width, height);

                    float curY = panelY + panelH - 25.0f * UI_SCALE;
                    textRenderer.renderText("[ SELECTED ]", panelX + 15.0f * UI_SCALE, curY, 0.9f * UI_SCALE, glm::vec3(0.0f, 0.8f, 1.0f), width, height);
                    curY -= 20.0f * UI_SCALE;

                    textRenderer.renderText(hudInfo.name, panelX + 15.0f * UI_SCALE, curY, 1.4f * UI_SCALE, glm::vec3(1.0f, 0.85f, 0.3f), width, height);
                    curY -= 12.0f * UI_SCALE;

                    textRenderer.renderQuad(panelX + 15.0f * UI_SCALE, curY, panelW - 30.0f * UI_SCALE, 1.0f, borderColor, width, height);
                    curY -= 18.0f * UI_SCALE;

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
                    textRenderer.renderText("TYPE", panelX + 15.0f * UI_SCALE, curY, 0.9f * UI_SCALE, glm::vec3(0.0f, 0.8f, 1.0f), width, height);
                    curY -= 15.0f * UI_SCALE;
                    float typeH = renderWrappedText(hudInfo.type, panelX + 15.0f * UI_SCALE, curY, panelW - 30.0f * UI_SCALE, 0.9f * UI_SCALE, glm::vec3(0.9f, 0.9f, 0.9f));
                    curY -= typeH + 10.0f * UI_SCALE;

                    // Features Section
                    textRenderer.renderText("FEATURES", panelX + 15.0f * UI_SCALE, curY, 0.9f * UI_SCALE, glm::vec3(0.0f, 0.8f, 1.0f), width, height);
                    curY -= 15.0f * UI_SCALE;
                    float featH = renderWrappedText(hudInfo.features, panelX + 15.0f * UI_SCALE, curY, panelW - 30.0f * UI_SCALE, 0.9f * UI_SCALE, glm::vec3(0.9f, 0.9f, 0.9f));
                    curY -= featH + 10.0f * UI_SCALE;

                    // Moons Section
                    textRenderer.renderText("MOONS INCLUDED", panelX + 15.0f * UI_SCALE, curY, 0.9f * UI_SCALE, glm::vec3(0.0f, 0.8f, 1.0f), width, height);
                    curY -= 15.0f * UI_SCALE;
                    float moonH = renderWrappedText(hudInfo.moons, panelX + 15.0f * UI_SCALE, curY, panelW - 30.0f * UI_SCALE, 0.9f * UI_SCALE, glm::vec3(0.9f, 0.9f, 0.9f));
                    curY -= moonH + 10.0f * UI_SCALE;

                    // Concepts Section
                    textRenderer.renderText("CG CONCEPTS DEMONSTRATED", panelX + 15.0f * UI_SCALE, curY, 0.9f * UI_SCALE, glm::vec3(0.0f, 0.8f, 1.0f), width, height);
                    curY -= 15.0f * UI_SCALE;
                    renderWrappedText(hudInfo.concepts, panelX + 15.0f * UI_SCALE, curY, panelW - 30.0f * UI_SCALE, 0.9f * UI_SCALE, glm::vec3(0.9f, 0.9f, 0.9f));
                }
            }

            // 7. --- TOP-CENTER ISS STATION PANEL (Manual mode only) ---
            if (currentDemo == DemoStage::NONE && sceneManager.getActiveCamera() && earth) {
                glm::vec3 camPos = sceneManager.getActiveCamera()->getPosition();
                glm::vec3 earthPos = earth->getPosition();
                float dist = glm::distance(camPos, earthPos);
                if (dist < 2.0f) {
                    std::string label1 = "ISS-style Space Station";
                    std::string label2 = "Low Earth Orbit";
                    std::string label3 = "Built from hierarchical transformations";
                    
                    float labelScale = 1.3f * UI_SCALE;
                    float subScale = 1.0f * UI_SCALE;
                    
                    float sW = 320.0f * UI_SCALE;
                    float sH = 75.0f * UI_SCALE;
                    float sX = (width - sW) / 2.0f;
                    float sY = height - sH - 90.0f * UI_SCALE;
                    
                    glm::vec4 sBorder = glm::vec4(0.0f, 0.9f, 1.0f, 0.4f);
                    textRenderer.renderQuad(sX - 1.0f, sY - 1.0f, sW + 2.0f, sH + 2.0f, sBorder, width, height);
                    textRenderer.renderQuad(sX, sY, sW, sH, manualBgCol, width, height);
                    
                    float yOffset = sY + sH - 22.0f * UI_SCALE;
                    textRenderer.renderText(label1, sX + (sW - label1.length() * 8.0f * labelScale) / 2.0f, yOffset, labelScale, glm::vec3(0.0f, 0.9f, 1.0f), width, height);
                    yOffset -= 20.0f * UI_SCALE;
                    textRenderer.renderText(label2, sX + (sW - label2.length() * 8.0f * subScale) / 2.0f, yOffset, subScale, glm::vec3(0.8f, 0.8f, 0.8f), width, height);
                    yOffset -= 17.0f * UI_SCALE;
                    textRenderer.renderText(label3, sX + (sW - label3.length() * 8.0f * subScale) / 2.0f, yOffset, subScale, glm::vec3(1.0f, 0.85f, 0.3f), width, height);
                }
            }

            // 8. --- CENTER PAUSE INDICATOR (When simulation or demo is paused) ---
            if (animationPaused) {
                std::string pauseText = (currentDemo == DemoStage::NONE) ? "PAUSED" : "DEMO PAUSED";
                float pauseScale = 1.8f * UI_SCALE;
                glm::vec3 pauseColor = (currentDemo == DemoStage::NONE) ? glm::vec3(1.0f, 0.5f, 0.0f) : glm::vec3(1.0f, 0.35f, 0.0f);
                
                float cardW = (pauseText.length() * 8.0f) * pauseScale + 40.0f * UI_SCALE;
                float cardH = 8.0f * pauseScale + 24.0f * UI_SCALE;
                float cardX = (width - cardW) / 2.0f;
                float cardY = (height - cardH) / 2.0f;
                
                glm::vec4 pauseBorder = glm::vec4(1.0f, 0.5f, 0.0f, 0.4f);
                glm::vec4 pauseBg = glm::vec4(0.08f, 0.05f, 0.02f, 0.85f);
                textRenderer.renderQuad(cardX - 1.0f, cardY - 1.0f, cardW + 2.0f, cardH + 2.0f, pauseBorder, width, height);
                textRenderer.renderQuad(cardX, cardY, cardW, cardH, pauseBg, width, height);
                
                textRenderer.renderText(pauseText, cardX + 20.0f * UI_SCALE, cardY + 12.0f * UI_SCALE, pauseScale, pauseColor, width, height);
            }
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

                char frameMsBuf[32];
                std::snprintf(frameMsBuf, sizeof(frameMsBuf), "%.2f", displayFrameMs);

                std::string title = "Solar System | FPS: " + fps_str +
                                    " | " + frameMsBuf + " ms" +
                                    " | Speed: " + speed_str + "x | " + camName +
                                    " | VSync: " + std::string(window.isVSyncEnabled() ? "ON" : "OFF") +
                                    " | Bloom: " + std::string(bloomEnabled ? "ON" : "OFF") +
                                    demoSuffix;
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
