#include "celestial/SpaceStation.h"
#include "celestial/OrbitMath.h"
#include "celestial/Planet.h"
#include "core/Renderer.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <cmath>
#include <iostream>

namespace {
constexpr float kStationDebugBoundsRadius = 0.18f;
}

SpaceStation::SpaceStation(const std::string& name,
                           float orbitRadius,
                           float orbitSpeed,
                           std::shared_ptr<Planet> parentPlanet,
                           float orbitPhaseOffset,
                           float orbitInclination,
                           float longitudeOfAscendingNode)
    : SceneObject(name), m_orbitRadius(orbitRadius), m_orbitSpeed(orbitSpeed),
      m_orbitInclination(orbitInclination), m_longitudeOfAscendingNode(longitudeOfAscendingNode),
      m_parentPlanet(parentPlanet), m_orbitAngle(orbitPhaseOffset), m_time(0.0f) {
    
    m_lastPos = calculateOrbitPosition(
        m_orbitRadius,
        m_orbitRadius,
        m_orbitInclination,
        m_longitudeOfAscendingNode,
        m_orbitAngle
    );

    // Initialize unit primitives
    m_cubeMesh = std::make_unique<Mesh>(createUnitCube());
    m_cylinderMesh = std::make_unique<Mesh>(createUnitCylinder(16));
    m_lightMesh = std::make_unique<Mesh>(Renderer::createSphere(0.002f, 6, 6));
}

void SpaceStation::update(float deltaTime) {
    m_orbitAngle += m_orbitSpeed * deltaTime;
    m_time += deltaTime;

    // Local orbital offset around Earth. The station stays in world space so
    // Earth's visual scale never shrinks the orbit radius.
    glm::vec3 relativePos = calculateOrbitPosition(
        m_orbitRadius,
        m_orbitRadius,
        m_orbitInclination,
        m_longitudeOfAscendingNode,
        m_orbitAngle
    );

    glm::vec3 parentPos = m_parentPlanet ? m_parentPlanet->getPosition() : glm::vec3(0.0f);
    glm::vec3 worldPos = parentPos + relativePos;

    m_transform.setPosition(worldPos);

    if (m_time > 0.01f) {
        glm::vec3 deltaPos = relativePos - m_lastPos;
        if (glm::dot(deltaPos, deltaPos) > 0.000001f) {
            m_velocity = glm::normalize(deltaPos);
        }
    }
    m_lastPos = relativePos;

    glm::vec3 sunDir = -glm::normalize(worldPos);
    m_sunTrackAngle = std::atan2(sunDir.x, sunDir.z);

    glm::vec3 upRef(0.0f, 1.0f, 0.0f);
    if (std::abs(glm::dot(m_velocity, upRef)) > 0.98f) {
        upRef = glm::vec3(1.0f, 0.0f, 0.0f);
    }

    glm::vec3 right = glm::normalize(glm::cross(upRef, m_velocity));
    glm::vec3 up = glm::normalize(glm::cross(m_velocity, right));
    glm::quat progradeRotation = glm::quat_cast(glm::mat3(right, up, m_velocity));
    m_transform.setRotation(glm::eulerAngles(progradeRotation));

#ifndef NDEBUG
    if (m_parentPlanet && !m_warnedAboutParentOverlap) {
        const float safeDistance = m_parentPlanet->getRadius() + kStationDebugBoundsRadius + 0.05f;
        const float actualDistance = glm::length(relativePos);
        if (actualDistance < safeDistance) {
            std::cerr << "Layout Warning: " << m_name
                      << " is inside the safe clearance around "
                      << m_parentPlanet->getName()
                      << " (distance=" << actualDistance
                      << ", required=" << safeDistance << ").\n";
            m_warnedAboutParentOverlap = true;
        }
    }
#endif
}

glm::vec3 SpaceStation::getCloseUpTargetPoint() const {
    glm::mat4 model = m_transform.getModelMatrix();
    // Offset target slightly along the truss (X) and forward modules (Z) for detailed camera tracking
    return glm::vec3(model * glm::vec4(0.008f, -0.005f, 0.012f, 1.0f));
}

void SpaceStation::render(Renderer& renderer) {
    if (!m_cubeMesh || !m_cylinderMesh) return;

    float scaleFactor = 2.8f; // Fixed scale: no camera-distance scaling.
    glm::mat4 modelBase = m_transform.getModelMatrix();
    modelBase = glm::scale(modelBase, glm::vec3(scaleFactor));

    const Shader& shader = renderer.getShader();
    shader.use();

    // Disable texture mapping to use color overrides
    shader.setBool("useTexture", false);
    shader.setBool("useColorOverride", true);
    shader.setInt("planetId", 200); // Trigger custom white/blue rim light effect in cube.frag

    // Reusable lambdas for rendering primitives hierarchically
    auto renderCylinder = [&](const glm::mat4& parentMat, const glm::vec3& translate, const glm::vec3& scale, const glm::vec3& rotationEuler, const glm::vec3& color) {
        glm::mat4 m = parentMat;
        m = glm::translate(m, translate);
        if (rotationEuler.x != 0.0f) m = glm::rotate(m, rotationEuler.x, glm::vec3(1.0f, 0.0f, 0.0f));
        if (rotationEuler.y != 0.0f) m = glm::rotate(m, rotationEuler.y, glm::vec3(0.0f, 1.0f, 0.0f));
        if (rotationEuler.z != 0.0f) m = glm::rotate(m, rotationEuler.z, glm::vec3(0.0f, 0.0f, 1.0f));
        m = glm::scale(m, scale);
        shader.setVec3("colorOverride", color);
        renderer.renderWithLighting(*m_cylinderMesh, shader, m);
    };

    auto renderCube = [&](const glm::mat4& parentMat, const glm::vec3& translate, const glm::vec3& scale, const glm::vec3& rotationEuler, const glm::vec3& color) {
        glm::mat4 m = parentMat;
        m = glm::translate(m, translate);
        if (rotationEuler.x != 0.0f) m = glm::rotate(m, rotationEuler.x, glm::vec3(1.0f, 0.0f, 0.0f));
        if (rotationEuler.y != 0.0f) m = glm::rotate(m, rotationEuler.y, glm::vec3(0.0f, 1.0f, 0.0f));
        if (rotationEuler.z != 0.0f) m = glm::rotate(m, rotationEuler.z, glm::vec3(0.0f, 0.0f, 1.0f));
        m = glm::scale(m, scale);
        shader.setVec3("colorOverride", color);
        renderer.renderWithLighting(*m_cubeMesh, shader, m);
    };

    // Color Palette
    glm::vec3 whiteMod = glm::vec3(0.88f, 0.88f, 0.90f);
    glm::vec3 greyMod = glm::vec3(0.60f, 0.62f, 0.65f);
    glm::vec3 darkTruss = glm::vec3(0.35f, 0.35f, 0.38f);
    glm::vec3 goldMetal = glm::vec3(0.85f, 0.65f, 0.18f);
    glm::vec3 jointColor = glm::vec3(0.45f, 0.45f, 0.47f);
    glm::vec3 darkBlue = glm::vec3(0.05f, 0.15f, 0.40f);
    glm::vec3 gridColor = glm::vec3(0.02f, 0.02f, 0.03f);

    // =========================================================================
    // 1. CENTRAL LONG TRUSS STRUCTURE
    // =========================================================================
    // Main structural spine (cube box along X)
    renderCube(modelBase, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.14f, 0.004f, 0.004f), glm::vec3(0.0f), darkTruss);
    
    // Cross joints and reinforcement braces along the truss
    for (float x = -0.05f; x <= 0.05f; x += 0.025f) {
        renderCube(modelBase, glm::vec3(x, 0.0f, 0.0f), glm::vec3(0.003f, 0.006f, 0.006f), glm::vec3(0.0f), jointColor);
        // Small diagonal beams
        renderCube(modelBase, glm::vec3(x + 0.0125f, 0.0f, 0.0f), glm::vec3(0.001f, 0.005f, 0.005f), glm::vec3(0.0f, 0.0f, 0.785f), jointColor);
    }
    
    // Truss extensions (left/right cylinders along X)
    renderCylinder(modelBase, glm::vec3(-0.075f, 0.0f, 0.0f), glm::vec3(0.003f, 0.012f, 0.003f), glm::vec3(0.0f, 0.0f, 1.5708f), jointColor);
    renderCylinder(modelBase, glm::vec3(0.075f, 0.0f, 0.0f), glm::vec3(0.003f, 0.012f, 0.003f), glm::vec3(0.0f, 0.0f, 1.5708f), jointColor);

    // =========================================================================
    // 2. PRESSURIZED HABITATION AND LAB MODULES
    // =========================================================================
    // Translate down slightly to represent modules hanging from the central truss
    glm::mat4 modulesBase = glm::translate(modelBase, glm::vec3(0.0f, -0.008f, 0.0f));

    // A. Unity Node 1 (Central hub)
    // Cylinder along Z (rotated 90 deg around X)
    renderCylinder(modulesBase, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.011f, 0.014f, 0.011f), glm::vec3(1.5708f, 0.0f, 0.0f), greyMod);

    // B. Destiny Laboratory (US Laboratory module, forward of Unity)
    renderCylinder(modulesBase, glm::vec3(0.0f, 0.0f, 0.018f), glm::vec3(0.011f, 0.024f, 0.011f), glm::vec3(1.5708f, 0.0f, 0.0f), whiteMod);

    // C. Harmony Node 2 (Forward connecting node)
    renderCylinder(modulesBase, glm::vec3(0.0f, 0.0f, 0.035f), glm::vec3(0.011f, 0.012f, 0.011f), glm::vec3(1.5708f, 0.0f, 0.0f), whiteMod);

    // D. Columbus Laboratory (ESA module, starboard of Node 2)
    renderCylinder(modulesBase, glm::vec3(0.013f, 0.0f, 0.035f), glm::vec3(0.010f, 0.016f, 0.010f), glm::vec3(0.0f, 0.0f, 1.5708f), whiteMod);

    // E. Kibo Laboratory (JAXA module, port of Node 2)
    renderCylinder(modulesBase, glm::vec3(-0.015f, 0.0f, 0.035f), glm::vec3(0.010f, 0.022f, 0.010f), glm::vec3(0.0f, 0.0f, 1.5708f), whiteMod);
    // Kibo exposed facility porch (small flat tray)
    renderCube(modulesBase, glm::vec3(-0.030f, -0.002f, 0.035f), glm::vec3(0.008f, 0.002f, 0.010f), glm::vec3(0.0f), greyMod);

    // F. Quest Joint Airlock (Starboard of Unity Node 1)
    renderCylinder(modulesBase, glm::vec3(0.010f, 0.0f, 0.0f), glm::vec3(0.008f, 0.010f, 0.008f), glm::vec3(0.0f, 0.0f, 1.5708f), whiteMod);
    // Outer gas tanks (small gold spheres/cylinders)
    renderCylinder(modulesBase, glm::vec3(0.010f, 0.005f, 0.003f), glm::vec3(0.003f, 0.004f, 0.003f), glm::vec3(0.0f), goldMetal);
    renderCylinder(modulesBase, glm::vec3(0.010f, -0.005f, -0.003f), glm::vec3(0.003f, 0.004f, 0.003f), glm::vec3(0.0f), goldMetal);

    // G. Tranquility Node 3 (Port of Unity Node 1)
    renderCylinder(modulesBase, glm::vec3(-0.010f, 0.0f, 0.0f), glm::vec3(0.011f, 0.012f, 0.011f), glm::vec3(0.0f, 0.0f, 1.5708f), whiteMod);
    
    // H. Cupola (Earth-facing observatory, nadir of Tranquility)
    renderCylinder(modulesBase, glm::vec3(-0.010f, -0.007f, 0.0f), glm::vec3(0.006f, 0.004f, 0.006f), glm::vec3(0.0f), jointColor);

    // I. Zarya FGB (Russian Cargo Module, aft of Unity Node 1)
    renderCylinder(modulesBase, glm::vec3(0.0f, 0.0f, -0.018f), glm::vec3(0.011f, 0.024f, 0.011f), glm::vec3(1.5708f, 0.0f, 0.0f), greyMod);
    // Gold band details representing thermal blankets
    renderCylinder(modulesBase, glm::vec3(0.0f, 0.0f, -0.018f), glm::vec3(0.0115f, 0.003f, 0.0115f), glm::vec3(1.5708f, 0.0f, 0.0f), goldMetal);

    // J. Zvezda Service Module (Russian habitation module, aft-most)
    renderCylinder(modulesBase, glm::vec3(0.0f, 0.0f, -0.041f), glm::vec3(0.010f, 0.024f, 0.010f), glm::vec3(1.5708f, 0.0f, 0.0f), greyMod);
    // Zvezda's small local solar panels
    renderCube(modulesBase, glm::vec3(-0.022f, 0.0f, -0.041f), glm::vec3(0.015f, 0.0003f, 0.005f), glm::vec3(0.0f, 0.0f, 0.08f), darkBlue);
    renderCube(modulesBase, glm::vec3(0.022f, 0.0f, -0.041f), glm::vec3(0.015f, 0.0003f, 0.005f), glm::vec3(0.0f, 0.0f, -0.08f), darkBlue);

    // =========================================================================
    // 3. SOLAR ARRAY WINGS (SAW) - 4 LARGE RECTANGULAR ARRAYS
    // =========================================================================
    // Render the four major arrays (two left, two right)
    auto renderSolarArrayWing = [&](float xOffset, bool isLeft) {
        // Joint connector from truss to array
        renderCylinder(modelBase, glm::vec3(xOffset, 0.0f, 0.0f), glm::vec3(0.003f, 0.006f, 0.003f), glm::vec3(1.5708f, 0.0f, 0.0f), jointColor);

        // Central structural mast (golden-brown truss line running out along Z)
        renderCylinder(modelBase, glm::vec3(xOffset, 0.0f, 0.0f), glm::vec3(0.0012f, 0.076f, 0.0012f), glm::vec3(1.5708f, 0.0f, 0.0f), goldMetal);

        // Rotate around the mast axis to track the Sun at the scene origin.
        float panelRot = m_sunTrackAngle + (isLeft ? 0.0f : 1.57f);

        glm::mat4 panelBase = glm::translate(modelBase, glm::vec3(xOffset, 0.0f, 0.0f));
        panelBase = glm::rotate(panelBase, panelRot, glm::vec3(0.0f, 0.0f, 1.0f));

        // Two solar blanket panels extending along +Z and -Z from center mast
        // Blanket 1 (+Z): centered at Z = 0.020f, size X = 0.016f, Y = 0.0004f, Z = 0.034f
        renderCube(panelBase, glm::vec3(0.0f, 0.0f, 0.020f), glm::vec3(0.016f, 0.0004f, 0.034f), glm::vec3(0.0f), darkBlue);
        // Blanket 2 (-Z): centered at Z = -0.020f, size X = 0.016f, Y = 0.0004f, Z = 0.034f
        renderCube(panelBase, glm::vec3(0.0f, 0.0f, -0.020f), glm::vec3(0.016f, 0.0004f, 0.034f), glm::vec3(0.0f), darkBlue);

        // Grid lines (thin black decal boxes placed slightly above & below blanket surfaces)
        float yOffset = 0.00045f;
        for (int side = -1; side <= 1; side += 2) {
            float y = side * yOffset;

            // Blanket 1 Grid Lines
            // Center separator (vertical along Z)
            renderCube(panelBase, glm::vec3(0.0f, y, 0.020f), glm::vec3(0.0008f, 0.0001f, 0.034f), glm::vec3(0.0f), gridColor);
            // Horizontal segment lines (along X)
            for (float zVal = 0.006f; zVal <= 0.035f; zVal += 0.007f) {
                renderCube(panelBase, glm::vec3(0.0f, y, zVal), glm::vec3(0.016f, 0.0001f, 0.0006f), glm::vec3(0.0f), gridColor);
            }

            // Blanket 2 Grid Lines
            // Center separator (vertical along Z)
            renderCube(panelBase, glm::vec3(0.0f, y, -0.020f), glm::vec3(0.0008f, 0.0001f, 0.034f), glm::vec3(0.0f), gridColor);
            // Horizontal segment lines (along X)
            for (float zVal = -0.006f; zVal >= -0.035f; zVal -= 0.007f) {
                renderCube(panelBase, glm::vec3(0.0f, y, zVal), glm::vec3(0.016f, 0.0001f, 0.0006f), glm::vec3(0.0f), gridColor);
            }
        }
    };

    // Render the 4 wings:
    renderSolarArrayWing(-0.082f, true);  // Left Outer
    renderSolarArrayWing(-0.058f, true);  // Left Inner
    renderSolarArrayWing(0.058f, false);  // Right Inner
    renderSolarArrayWing(0.082f, false);  // Right Outer

    // =========================================================================
    // 4. DOCKED SOYUZ CREW VEHICLE
    // =========================================================================
    // Docked at the aft port of the Zvezda module (Z = -0.053f relative to modulesBase)
    glm::mat4 soyuzBase = glm::translate(modulesBase, glm::vec3(0.0f, 0.0f, -0.054f));

    // Docking probe connector
    renderCylinder(soyuzBase, glm::vec3(0.0f, 0.0f, -0.001f), glm::vec3(0.003f, 0.002f, 0.003f), glm::vec3(1.5708f, 0.0f, 0.0f), jointColor);
    // Spherical Orbital Module
    renderCylinder(soyuzBase, glm::vec3(0.0f, 0.0f, -0.004f), glm::vec3(0.005f, 0.004f, 0.005f), glm::vec3(1.5708f, 0.0f, 0.0f), greyMod);
    // Bell-shaped Descent Module (dark greenish grey)
    renderCylinder(soyuzBase, glm::vec3(0.0f, 0.0f, -0.0075f), glm::vec3(0.0055f, 0.0035f, 0.0055f), glm::vec3(1.5708f, 0.0f, 0.0f), glm::vec3(0.32f, 0.40f, 0.32f));
    // Instrument/Service Module (grey cylinder)
    renderCylinder(soyuzBase, glm::vec3(0.0f, 0.0f, -0.0125f), glm::vec3(0.006f, 0.006f, 0.006f), glm::vec3(1.5708f, 0.0f, 0.0f), greyMod);
    // Service Module flared base
    renderCylinder(soyuzBase, glm::vec3(0.0f, 0.0f, -0.016f), glm::vec3(0.007f, 0.002f, 0.007f), glm::vec3(1.5708f, 0.0f, 0.0f), jointColor);
    // Two small Soyuz solar panel wings extending port & starboard (along X-axis)
    renderCube(soyuzBase, glm::vec3(-0.012f, 0.0f, -0.0125f), glm::vec3(0.015f, 0.0002f, 0.004f), glm::vec3(0.0f), darkBlue);
    renderCube(soyuzBase, glm::vec3(0.012f, 0.0f, -0.0125f), glm::vec3(0.015f, 0.0002f, 0.004f), glm::vec3(0.0f), darkBlue);

    // =========================================================================
    // 5. CANADARM2 ROBOTIC ARM
    // =========================================================================
    // Mounted on US Lab Destiny (modulesBase, Z = 0.018f, top-port side)
    glm::mat4 armBase = glm::translate(modulesBase, glm::vec3(-0.006f, 0.005f, 0.018f));
    // Rotate elbow joint based on time for dynamic articulation
    float elbowAngle = glm::radians(-75.0f) + 0.15f * std::sin(m_time * 0.4f);
    
    // Joint base
    renderCube(armBase, glm::vec3(0.0f), glm::vec3(0.003f, 0.003f, 0.003f), glm::vec3(0.0f), jointColor);
    
    // Link 1 (Lower arm boom): points up & slightly left
    glm::mat4 link1 = armBase;
    link1 = glm::rotate(link1, glm::radians(35.0f), glm::vec3(0.0f, 0.0f, 1.0f)); // shoulder roll
    link1 = glm::rotate(link1, glm::radians(20.0f), glm::vec3(1.0f, 0.0f, 0.0f)); // shoulder pitch
    renderCylinder(link1, glm::vec3(0.0f, 0.006f, 0.0f), glm::vec3(0.001f, 0.012f, 0.001f), glm::vec3(0.0f), whiteMod);

    // Elbow Joint
    glm::mat4 elbowMat = glm::translate(link1, glm::vec3(0.0f, 0.012f, 0.0f));
    renderCylinder(elbowMat, glm::vec3(0.0f), glm::vec3(0.0018f, 0.0018f, 0.0018f), glm::vec3(1.5708f, 0.0f, 0.0f), jointColor);

    // Link 2 (Upper arm boom): bent relative to link 1
    glm::mat4 link2 = glm::rotate(elbowMat, elbowAngle, glm::vec3(1.0f, 0.0f, 0.0f));
    renderCylinder(link2, glm::vec3(0.0f, 0.005f, 0.0f), glm::vec3(0.001f, 0.010f, 0.001f), glm::vec3(0.0f), whiteMod);

    // End effector gripper
    renderCylinder(link2, glm::vec3(0.0f, 0.0105f, 0.0f), glm::vec3(0.0014f, 0.0015f, 0.0014f), glm::vec3(0.0f), jointColor);

    // =========================================================================
    // 6. COMMUNICATION ANTENNA ARRAY
    // =========================================================================
    // Attached on top of Zvezda module
    glm::mat4 antBase = glm::translate(modulesBase, glm::vec3(0.0f, 0.005f, -0.045f));
    
    // Antenna boom mast pointing up and aft
    glm::mat4 antMast = antBase;
    antMast = glm::rotate(antMast, glm::radians(30.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    renderCylinder(antMast, glm::vec3(0.0f, 0.006f, 0.0f), glm::vec3(0.0008f, 0.012f, 0.0008f), glm::vec3(0.0f), jointColor);

    // Dish mount
    glm::mat4 antDishPivot = glm::translate(antMast, glm::vec3(0.0f, 0.012f, 0.0f));
    renderCylinder(antDishPivot, glm::vec3(0.0f), glm::vec3(0.0015f, 0.0015f, 0.0015f), glm::vec3(0.0f, 0.0f, 1.5708f), jointColor);

    // Gold Antenna Dish
    glm::mat4 antDish = glm::rotate(antDishPivot, glm::radians(-60.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    renderCylinder(antDish, glm::vec3(0.0f, 0.0006f, 0.0f), glm::vec3(0.007f, 0.0006f, 0.007f), glm::vec3(0.0f), goldMetal);

    bool blink = (std::fmod(m_time, 2.0f) < 1.0f);
    if (blink && m_lightMesh) {
        auto renderLight = [&](const glm::vec3& offset, const glm::vec3& color) {
            glm::mat4 lm = glm::translate(modelBase, offset);
            lm = glm::scale(lm, glm::vec3(1.5f));
            shader.setVec3("colorOverride", color);
            renderer.renderWithLighting(*m_lightMesh, shader, lm);
        };

        renderLight(glm::vec3(-0.085f, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        renderLight(glm::vec3(0.085f, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        renderLight(glm::vec3(0.0f, 0.0f, 0.040f), glm::vec3(1.0f, 1.0f, 1.0f));
        renderLight(glm::vec3(0.0f, 0.0f, -0.060f), glm::vec3(1.0f, 0.5f, 0.0f));
    }

    // Restore shader states
    shader.setBool("useColorOverride", false);
}

Mesh SpaceStation::createUnitCube() {
    std::vector<Vertex> vertices = {
        // Front face (normal 0, 0, 1)
        { glm::vec3(-0.5f, -0.5f,  0.5f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(1.0f), glm::vec2(0.0f, 0.0f) },
        { glm::vec3( 0.5f, -0.5f,  0.5f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(1.0f), glm::vec2(1.0f, 0.0f) },
        { glm::vec3( 0.5f,  0.5f,  0.5f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(1.0f), glm::vec2(1.0f, 1.0f) },
        { glm::vec3(-0.5f,  0.5f,  0.5f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(1.0f), glm::vec2(0.0f, 1.0f) },

        // Back face (normal 0, 0, -1)
        { glm::vec3(-0.5f, -0.5f, -0.5f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(1.0f), glm::vec2(1.0f, 0.0f) },
        { glm::vec3(-0.5f,  0.5f, -0.5f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(1.0f), glm::vec2(1.0f, 1.0f) },
        { glm::vec3( 0.5f,  0.5f, -0.5f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(1.0f), glm::vec2(0.0f, 1.0f) },
        { glm::vec3( 0.5f, -0.5f, -0.5f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(1.0f), glm::vec2(0.0f, 0.0f) },

        // Top face (normal 0, 1, 0)
        { glm::vec3(-0.5f,  0.5f, -0.5f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(1.0f), glm::vec2(0.0f, 1.0f) },
        { glm::vec3(-0.5f,  0.5f,  0.5f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(1.0f), glm::vec2(0.0f, 0.0f) },
        { glm::vec3( 0.5f,  0.5f,  0.5f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(1.0f), glm::vec2(1.0f, 0.0f) },
        { glm::vec3( 0.5f,  0.5f, -0.5f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(1.0f), glm::vec2(1.0f, 1.0f) },

        // Bottom face (normal 0, -1, 0)
        { glm::vec3(-0.5f, -0.5f, -0.5f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(1.0f), glm::vec2(0.0f, 0.0f) },
        { glm::vec3( 0.5f, -0.5f, -0.5f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(1.0f), glm::vec2(1.0f, 0.0f) },
        { glm::vec3( 0.5f, -0.5f,  0.5f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(1.0f), glm::vec2(1.0f, 1.0f) },
        { glm::vec3(-0.5f, -0.5f,  0.5f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(1.0f), glm::vec2(0.0f, 1.0f) },

        // Right face (normal 1, 0, 0)
        { glm::vec3( 0.5f, -0.5f, -0.5f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(1.0f), glm::vec2(1.0f, 0.0f) },
        { glm::vec3( 0.5f,  0.5f, -0.5f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(1.0f), glm::vec2(1.0f, 1.0f) },
        { glm::vec3( 0.5f,  0.5f,  0.5f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(1.0f), glm::vec2(0.0f, 1.0f) },
        { glm::vec3( 0.5f, -0.5f,  0.5f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(1.0f), glm::vec2(0.0f, 0.0f) },

        // Left face (normal -1, 0, 0)
        { glm::vec3(-0.5f, -0.5f, -0.5f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(1.0f), glm::vec2(0.0f, 0.0f) },
        { glm::vec3(-0.5f, -0.5f,  0.5f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(1.0f), glm::vec2(1.0f, 0.0f) },
        { glm::vec3(-0.5f,  0.5f,  0.5f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(1.0f), glm::vec2(1.0f, 1.0f) },
        { glm::vec3(-0.5f,  0.5f, -0.5f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(1.0f), glm::vec2(0.0f, 1.0f) }
    };

    std::vector<unsigned int> indices = {
        0, 1, 2,  2, 3, 0,
        4, 5, 6,  6, 7, 4,
        8, 9, 10, 10, 11, 8,
        12, 13, 14, 14, 15, 12,
        16, 17, 18, 18, 19, 16,
        20, 21, 22, 22, 23, 20
    };

    return Mesh(vertices, indices);
}

Mesh SpaceStation::createUnitCylinder(unsigned int segments) {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    const float PI = 3.14159265359f;
    float halfHeight = 0.5f;
    float radius = 0.5f; // diameter = 1.0

    // 1. Side wall vertices
    for (unsigned int i = 0; i <= segments; ++i) {
        float angle = 2.0f * PI * i / segments;
        float x = std::cos(angle);
        float z = std::sin(angle);

        // Bottom wall vertex
        Vertex vBot;
        vBot.position = glm::vec3(x * radius, -halfHeight, z * radius);
        vBot.normal = glm::vec3(x, 0.0f, z);
        vBot.color = glm::vec3(1.0f);
        vBot.texCoords = glm::vec2((float)i / segments, 0.0f);
        vertices.push_back(vBot);

        // Top wall vertex
        Vertex vTop;
        vTop.position = glm::vec3(x * radius, halfHeight, z * radius);
        vTop.normal = glm::vec3(x, 0.0f, z);
        vTop.color = glm::vec3(1.0f);
        vTop.texCoords = glm::vec2((float)i / segments, 1.0f);
        vertices.push_back(vTop);
    }

    // Side wall indices
    for (unsigned int i = 0; i < segments; ++i) {
        unsigned int bot1 = 2 * i;
        unsigned int top1 = 2 * i + 1;
        unsigned int bot2 = 2 * (i + 1);
        unsigned int top2 = 2 * (i + 1) + 1;

        indices.push_back(bot1);
        indices.push_back(bot2);
        indices.push_back(top1);

        indices.push_back(top1);
        indices.push_back(bot2);
        indices.push_back(top2);
    }

    // 2. Bottom Cap
    unsigned int capOffset = vertices.size();
    
    Vertex vBotCenter;
    vBotCenter.position = glm::vec3(0.0f, -halfHeight, 0.0f);
    vBotCenter.normal = glm::vec3(0.0f, -1.0f, 0.0f);
    vBotCenter.color = glm::vec3(1.0f);
    vBotCenter.texCoords = glm::vec2(0.5f, 0.5f);
    vertices.push_back(vBotCenter);

    for (unsigned int i = 0; i <= segments; ++i) {
        float angle = 2.0f * PI * i / segments;
        float x = std::cos(angle);
        float z = std::sin(angle);

        Vertex v;
        v.position = glm::vec3(x * radius, -halfHeight, z * radius);
        v.normal = glm::vec3(0.0f, -1.0f, 0.0f);
        v.color = glm::vec3(1.0f);
        v.texCoords = glm::vec2(0.5f + 0.5f * x, 0.5f + 0.5f * z);
        vertices.push_back(v);
    }

    for (unsigned int i = 0; i < segments; ++i) {
        indices.push_back(capOffset);
        indices.push_back(capOffset + 1 + i + 1);
        indices.push_back(capOffset + 1 + i);
    }

    // 3. Top Cap
    unsigned int topCapOffset = vertices.size();

    Vertex vTopCenter;
    vTopCenter.position = glm::vec3(0.0f, halfHeight, 0.0f);
    vTopCenter.normal = glm::vec3(0.0f, 1.0f, 0.0f);
    vTopCenter.color = glm::vec3(1.0f);
    vTopCenter.texCoords = glm::vec2(0.5f, 0.5f);
    vertices.push_back(vTopCenter);

    for (unsigned int i = 0; i <= segments; ++i) {
        float angle = 2.0f * PI * i / segments;
        float x = std::cos(angle);
        float z = std::sin(angle);

        Vertex v;
        v.position = glm::vec3(x * radius, halfHeight, z * radius);
        v.normal = glm::vec3(0.0f, 1.0f, 0.0f);
        v.color = glm::vec3(1.0f);
        v.texCoords = glm::vec2(0.5f + 0.5f * x, 0.5f + 0.5f * z);
        vertices.push_back(v);
    }

    for (unsigned int i = 0; i < segments; ++i) {
        indices.push_back(topCapOffset);
        indices.push_back(topCapOffset + 1 + i);
        indices.push_back(topCapOffset + 1 + i + 1);
    }

    return Mesh(vertices, indices);
}
