#include "celestial/Spacecraft.h"
#include "celestial/Planet.h"
#include "core/Renderer.h"
#include <glm/gtc/matrix_transform.hpp>
#include <GLFW/glfw3.h>
#include <cmath>
#include <cstdlib>

Spacecraft::Spacecraft(const std::string& name, std::shared_ptr<Planet> earth, std::shared_ptr<Planet> mars)
    : SceneObject(name), m_earth(earth), m_mars(mars), m_progress(0.0f), m_rollAngle(0.0f), m_position(0.0f), m_forward(0.0f, 0.0f, 1.0f) {
    
    // Initialize unit primitives
    m_cubeMesh = std::make_unique<Mesh>(createUnitCube());
    m_cylinderMesh = std::make_unique<Mesh>(createUnitCylinder(16));
    m_coneMesh = std::make_unique<Mesh>(createUnitCone(16));

    // Create trajectory mesh with 101 vertices (100 segments)
    std::vector<Vertex> trajVertices(101);
    std::vector<unsigned int> trajIndices;
    for (unsigned int i = 0; i <= 100; ++i) {
        trajIndices.push_back(i);
        trajVertices[i].position = glm::vec3(0.0f);
        trajVertices[i].normal = glm::vec3(0.0f, 1.0f, 0.0f);
        trajVertices[i].color = glm::vec3(0.0f, 0.8f, 1.0f); // Neon cyan path
        trajVertices[i].texCoords = glm::vec2((float)i / 100.0f, 0.0f);
    }
    m_trajectoryMesh = std::make_unique<Mesh>(trajVertices, trajIndices, GL_LINE_STRIP);

    // Initial update to place the spacecraft correctly at start
    update(0.0f);
}

glm::vec3 Spacecraft::getPositionForT(float t) const {
    if (!m_earth || !m_mars) return glm::vec3(0.0f);

    // Get current orbital parameters of Earth and Mars
    float eAngle = m_earth->getOrbitAngle();
    float eSpeed = m_earth->getOrbitSpeed();
    float mAngle = m_mars->getOrbitAngle();
    float mSpeed = m_mars->getOrbitSpeed();

    // Mission duration in simulation seconds (matching the 30-second loop)
    float T = 30.0f;

    // We want the trajectory to be defined by departure from Earth at t=0 (offset by -m_progress * T)
    // and arrival at Mars at t=1 (offset by +(1.0f - m_progress) * T)
    float dtDep = -m_progress * T;
    float dtArr = (1.0f - m_progress) * T;

    float angleDep = eAngle + eSpeed * dtDep;
    float angleArr = mAngle + mSpeed * dtArr;

    // Get 3D positions of the planets at departure and arrival times
    glm::vec3 P_dep = getPlanetPositionAtAngle(m_earth, angleDep);
    glm::vec3 P_arr = getPlanetPositionAtAngle(m_mars, angleArr);

    // Calculate polar angles in the XZ plane
    float phi_dep = std::atan2(P_dep.z, P_dep.x);
    float phi_arr = std::atan2(P_arr.z, P_arr.x);

    // To ensure prograde travel (counter-clockwise), make sure phi_arr > phi_dep
    float diff = phi_arr - phi_dep;
    while (diff < 0.1f) diff += 2.0f * 3.14159265359f;
    while (diff > 2.0f * 3.14159265359f + 0.1f) diff -= 2.0f * 3.14159265359f;

    // Interpolate the angle along the path
    float phi = phi_dep + t * diff;

    // Interpolate the radius (distance from Sun)
    float r_dep = glm::length(glm::vec3(P_dep.x, 0.0f, P_dep.z));
    float r_arr = glm::length(glm::vec3(P_arr.x, 0.0f, P_arr.z));
    float r = glm::mix(r_dep, r_arr, t);

    // Interpolate the height (y-coordinate)
    float y = glm::mix(P_dep.y, P_arr.y, t);

    // Compute the 3D position
    glm::vec3 pos;
    pos.x = r * std::cos(phi);
    pos.y = y;
    pos.z = r * std::sin(phi);

    return pos;
}

glm::vec3 Spacecraft::getTangentForT(float t) const {
    float delta = 0.001f;
    float t1 = glm::clamp(t - delta, 0.0f, 1.0f);
    float t2 = glm::clamp(t + delta, 0.0f, 1.0f);

    glm::vec3 p1 = getPositionForT(t1);
    glm::vec3 p2 = getPositionForT(t2);

    glm::vec3 dir = p2 - p1;
    if (glm::length(dir) > 0.0001f) {
        return glm::normalize(dir);
    }
    return glm::vec3(0.0f, 0.0f, 1.0f);
}

void Spacecraft::update(float deltaTime) {
    // 30 seconds for a complete roundtrip (speed = 1.0f / 30.0f)
    float speed = 0.0333f;
    m_progress += speed * deltaTime;
    if (m_progress > 1.0f) {
        m_progress = 0.0f; // restart mission
    }

    // Accumulate subtle roll rotation over time (barbecue roll)
    m_rollAngle += 0.15f * deltaTime;

    m_position = getPositionForT(m_progress);
    m_forward = getTangentForT(m_progress);

    m_transform.setPosition(m_position);

    // Calculate rotation to align local Z-axis (forward) with movement tangent and apply roll
    float pitch = -std::asin(m_forward.y);
    float yaw = std::atan2(m_forward.x, m_forward.z);
    m_transform.setRotation(glm::vec3(pitch, yaw, m_rollAngle));

    // Update trajectory line mesh vertices dynamically based on moving planets
    std::vector<Vertex> newVertices;
    for (int i = 0; i <= 100; ++i) {
        float t = (float)i / 100.0f;
        Vertex v;
        v.position = getPositionForT(t);
        v.normal = glm::vec3(0.0f, 1.0f, 0.0f);
        v.color = glm::vec3(0.0f, 0.8f, 1.0f);
        v.texCoords = glm::vec2(t, 0.0f);
        newVertices.push_back(v);
    }
    m_trajectoryMesh->updateVertices(newVertices);
}

void Spacecraft::render(Renderer& renderer) {
    if (!m_cubeMesh || !m_cylinderMesh || !m_coneMesh || !m_trajectoryMesh) return;

    const Shader& shader = renderer.getShader();
    shader.use();

    // --- 1. Draw Mission Trajectory Line (Cyan Glow, semi-transparent) ---
    shader.setBool("useTexture", false);
    shader.setBool("useColorOverride", true);
    shader.setVec3("colorOverride", glm::vec3(0.0f, 0.75f, 1.0f));
    shader.setFloat("emissiveStrength", 1.2f);
    shader.setFloat("globalAlpha", 0.4f);
    shader.setInt("planetId", -1);

    // Disable depth write temporarily to make the trajectory blend smoothly
    glDepthMask(GL_FALSE);
    renderer.render(*m_trajectoryMesh, shader, glm::mat4(1.0f));
    glDepthMask(GL_TRUE);

    shader.setFloat("emissiveStrength", 0.0f);
    shader.setFloat("globalAlpha", 1.0f);

    // --- 2. Draw Spacecraft Model Components ---
    glm::mat4 modelBase = m_transform.getModelMatrix();

    // 2a. Main Probe Body:
    // - Core: Gold-foil insulated cube at center
    glm::mat4 goldCoreModel = modelBase;
    goldCoreModel = glm::scale(goldCoreModel, glm::vec3(0.009f, 0.009f, 0.012f));
    shader.setVec3("colorOverride", glm::vec3(0.85f, 0.65f, 0.15f)); // gold foil
    renderer.renderWithLighting(*m_cubeMesh, shader, goldCoreModel);

    // - Scientific Instrument Ring: Silver cylinder forward of the core, aligned along Z
    glm::mat4 instrumentRingModel = modelBase;
    instrumentRingModel = glm::translate(instrumentRingModel, glm::vec3(0.0f, 0.0f, 0.007f));
    instrumentRingModel = glm::rotate(instrumentRingModel, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f)); // rotate local Y to Z
    instrumentRingModel = glm::scale(instrumentRingModel, glm::vec3(0.007f, 0.003f, 0.007f));
    shader.setVec3("colorOverride", glm::vec3(0.75f, 0.75f, 0.78f)); // silver/grey metal
    renderer.renderWithLighting(*m_cylinderMesh, shader, instrumentRingModel);

    // 2b. Cockpit / Sensor Head:
    // - Crew Cockpit Module: White cylinder at the very front
    glm::mat4 cockpitCabinModel = modelBase;
    cockpitCabinModel = glm::translate(cockpitCabinModel, glm::vec3(0.0f, 0.0f, 0.0115f));
    cockpitCabinModel = glm::rotate(cockpitCabinModel, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    cockpitCabinModel = glm::scale(cockpitCabinModel, glm::vec3(0.0055f, 0.005f, 0.0055f));
    shader.setVec3("colorOverride", glm::vec3(0.92f, 0.94f, 0.96f)); // clean white
    renderer.renderWithLighting(*m_cylinderMesh, shader, cockpitCabinModel);

    // - Visor Window: Cyan-tinted cube on top of the cockpit cabin
    glm::mat4 windowModel = modelBase;
    windowModel = glm::translate(windowModel, glm::vec3(0.0f, 0.0022f, 0.0125f)); // offset up and forward
    windowModel = glm::scale(windowModel, glm::vec3(0.0035f, 0.0018f, 0.0025f));
    shader.setVec3("colorOverride", glm::vec3(0.02f, 0.55f, 0.78f)); // glossy blue-cyan
    renderer.renderWithLighting(*m_cubeMesh, shader, windowModel);

    // - Sensor Turret Nose: Tiny dark grey cylinder at the nose tip
    glm::mat4 sensorNoseModel = modelBase;
    sensorNoseModel = glm::translate(sensorNoseModel, glm::vec3(0.0f, 0.0f, 0.015f));
    sensorNoseModel = glm::rotate(sensorNoseModel, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    sensorNoseModel = glm::scale(sensorNoseModel, glm::vec3(0.002f, 0.002f, 0.002f));
    shader.setVec3("colorOverride", glm::vec3(0.2f, 0.2f, 0.22f)); // dark carbon
    renderer.renderWithLighting(*m_cylinderMesh, shader, sensorNoseModel);

    // - Red Sensor Eye: Tiny red cube on the nose tip (glowing)
    glm::mat4 sensorEyeModel = modelBase;
    sensorEyeModel = glm::translate(sensorEyeModel, glm::vec3(0.0f, 0.0f, 0.016f));
    sensorEyeModel = glm::scale(sensorEyeModel, glm::vec3(0.0008f, 0.0008f, 0.0008f));
    shader.setVec3("colorOverride", glm::vec3(1.0f, 0.1f, 0.1f)); // crimson red
    shader.setFloat("emissiveStrength", 1.5f);
    renderer.render(*m_cubeMesh, shader, sensorEyeModel);
    shader.setFloat("emissiveStrength", 0.0f);

    // 2c. Left and Right Solar Panel Wings:
    // Left Mast
    glm::mat4 leftMastModel = modelBase;
    leftMastModel = glm::translate(leftMastModel, glm::vec3(-0.0075f, 0.0f, -0.002f));
    leftMastModel = glm::rotate(leftMastModel, glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f)); // align along X
    leftMastModel = glm::scale(leftMastModel, glm::vec3(0.001f, 0.006f, 0.001f));
    shader.setVec3("colorOverride", glm::vec3(0.55f, 0.55f, 0.58f));
    renderer.renderWithLighting(*m_cylinderMesh, shader, leftMastModel);

    // Left Panel (split into two segments for higher detail/realism)
    glm::mat4 leftPanelModel1 = modelBase;
    leftPanelModel1 = glm::translate(leftPanelModel1, glm::vec3(-0.0155f, 0.0f, -0.002f));
    leftPanelModel1 = glm::scale(leftPanelModel1, glm::vec3(0.008f, 0.0005f, 0.008f));
    shader.setVec3("colorOverride", glm::vec3(0.04f, 0.16f, 0.38f)); // deep solar panel blue
    renderer.renderWithLighting(*m_cubeMesh, shader, leftPanelModel1);

    glm::mat4 leftPanelModel2 = modelBase;
    leftPanelModel2 = glm::translate(leftPanelModel2, glm::vec3(-0.024f, 0.0f, -0.002f));
    leftPanelModel2 = glm::scale(leftPanelModel2, glm::vec3(0.008f, 0.0005f, 0.008f));
    shader.setVec3("colorOverride", glm::vec3(0.04f, 0.16f, 0.38f));
    renderer.renderWithLighting(*m_cubeMesh, shader, leftPanelModel2);

    // Right Mast
    glm::mat4 rightMastModel = modelBase;
    rightMastModel = glm::translate(rightMastModel, glm::vec3(0.0075f, 0.0f, -0.002f));
    rightMastModel = glm::rotate(rightMastModel, glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    rightMastModel = glm::scale(rightMastModel, glm::vec3(0.001f, 0.006f, 0.001f));
    shader.setVec3("colorOverride", glm::vec3(0.55f, 0.55f, 0.58f));
    renderer.renderWithLighting(*m_cylinderMesh, shader, rightMastModel);

    // Right Panel (split into two segments)
    glm::mat4 rightPanelModel1 = modelBase;
    rightPanelModel1 = glm::translate(rightPanelModel1, glm::vec3(0.0155f, 0.0f, -0.002f));
    rightPanelModel1 = glm::scale(rightPanelModel1, glm::vec3(0.008f, 0.0005f, 0.008f));
    shader.setVec3("colorOverride", glm::vec3(0.04f, 0.16f, 0.38f));
    renderer.renderWithLighting(*m_cubeMesh, shader, rightPanelModel1);

    glm::mat4 rightPanelModel2 = modelBase;
    rightPanelModel2 = glm::translate(rightPanelModel2, glm::vec3(0.024f, 0.0f, -0.002f));
    rightPanelModel2 = glm::scale(rightPanelModel2, glm::vec3(0.008f, 0.0005f, 0.008f));
    shader.setVec3("colorOverride", glm::vec3(0.04f, 0.16f, 0.38f));
    renderer.renderWithLighting(*m_cubeMesh, shader, rightPanelModel2);

    // 2d. Engine Nozzle:
    // - Dark carbon cone at the rear, wide end pointing backward
    glm::mat4 nozzleModel = modelBase;
    nozzleModel = glm::translate(nozzleModel, glm::vec3(0.0f, 0.0f, -0.009f)); // attach to rear of gold core
    nozzleModel = glm::rotate(nozzleModel, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    nozzleModel = glm::scale(nozzleModel, glm::vec3(0.006f, 0.006f, 0.006f));
    shader.setVec3("colorOverride", glm::vec3(0.22f, 0.22f, 0.25f)); // dark carbon grey
    renderer.renderWithLighting(*m_coneMesh, shader, nozzleModel);

    // 2e. Small Antenna Dish:
    // - Mast pointing upward
    glm::mat4 antennaMastModel = modelBase;
    antennaMastModel = glm::translate(antennaMastModel, glm::vec3(0.0f, 0.006f, 0.002f));
    antennaMastModel = glm::scale(antennaMastModel, glm::vec3(0.0008f, 0.006f, 0.0008f)); // thin cylinder along Y
    shader.setVec3("colorOverride", glm::vec3(0.6f, 0.6f, 0.62f));
    renderer.renderWithLighting(*m_cylinderMesh, shader, antennaMastModel);

    // - Flat cone dish on top, tilted forward
    glm::mat4 dishModel = modelBase;
    dishModel = glm::translate(dishModel, glm::vec3(0.0f, 0.009f, 0.002f));
    dishModel = glm::rotate(dishModel, glm::radians(60.0f), glm::vec3(1.0f, 0.0f, 0.0f)); // tilt forward
    dishModel = glm::scale(dishModel, glm::vec3(0.0045f, 0.0012f, 0.0045f));
    shader.setVec3("colorOverride", glm::vec3(0.82f, 0.82f, 0.84f)); // off-white grey
    renderer.renderWithLighting(*m_coneMesh, shader, dishModel);

    // 2f. Glowing Dual-Core Engine Exhaust Plume (semi-transparent)
    float timeVal = (float)glfwGetTime();
    float flicker = 1.0f + 0.12f * std::sin(timeVal * 26.0f);
    float lengthMod = 1.0f + 0.08f * std::cos(timeVal * 19.0f);

    // Disable depth write for transparent blending
    glDepthMask(GL_FALSE);

    // Outer plume: brilliant fiery orange cone expanding backward
    glm::mat4 outerExhaustModel = modelBase;
    outerExhaustModel = glm::translate(outerExhaustModel, glm::vec3(0.0f, 0.0f, -0.024f)); // centered further back
    outerExhaustModel = glm::rotate(outerExhaustModel, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    outerExhaustModel = glm::scale(outerExhaustModel, glm::vec3(0.0045f * flicker, 0.024f * lengthMod, 0.0045f * flicker));

    shader.setVec3("colorOverride", glm::vec3(1.0f, 0.45f, 0.02f)); // fiery orange
    shader.setFloat("emissiveStrength", 2.0f);
    shader.setFloat("globalAlpha", 0.45f);
    renderer.render(*m_coneMesh, shader, outerExhaustModel);

    // Inner core: intense hot cyan/blue cone
    glm::mat4 innerExhaustModel = modelBase;
    innerExhaustModel = glm::translate(innerExhaustModel, glm::vec3(0.0f, 0.0f, -0.019f)); // shorter, closer to nozzle
    innerExhaustModel = glm::rotate(innerExhaustModel, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    innerExhaustModel = glm::scale(innerExhaustModel, glm::vec3(0.0022f * flicker, 0.014f * lengthMod, 0.0022f * flicker));

    shader.setVec3("colorOverride", glm::vec3(0.0f, 0.85f, 1.0f)); // intense cyan-blue core
    shader.setFloat("emissiveStrength", 3.0f);
    shader.setFloat("globalAlpha", 0.8f);
    renderer.render(*m_coneMesh, shader, innerExhaustModel);

    // Re-enable depth write and reset states
    glDepthMask(GL_TRUE);
    shader.setFloat("emissiveStrength", 0.0f);
    shader.setFloat("globalAlpha", 1.0f);
    shader.setBool("useColorOverride", false);
}

Mesh Spacecraft::createUnitCube() {
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

Mesh Spacecraft::createUnitCylinder(unsigned int segments) {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    const float PI = 3.14159265359f;
    float halfHeight = 0.5f;
    float radius = 0.5f;

    // 1. Side wall vertices
    for (unsigned int i = 0; i <= segments; ++i) {
        float angle = 2.0f * PI * i / segments;
        float x = std::cos(angle);
        float z = std::sin(angle);

        Vertex vBot;
        vBot.position = glm::vec3(x * radius, -halfHeight, z * radius);
        vBot.normal = glm::vec3(x, 0.0f, z);
        vBot.color = glm::vec3(1.0f);
        vBot.texCoords = glm::vec2((float)i / segments, 0.0f);
        vertices.push_back(vBot);

        Vertex vTop;
        vTop.position = glm::vec3(x * radius, halfHeight, z * radius);
        vTop.normal = glm::vec3(x, 0.0f, z);
        vTop.color = glm::vec3(1.0f);
        vTop.texCoords = glm::vec2((float)i / segments, 1.0f);
        vertices.push_back(vTop);
    }

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

glm::vec3 Spacecraft::getPlanetPositionAtAngle(std::shared_ptr<Planet> planet, float angle) const {
    if (!planet) return glm::vec3(0.0f);

    float theta = angle;
    float iRad = glm::radians(planet->getInclination());
    float omegaRad = glm::radians(planet->getLongitudeOfAscendingNode());

    // 1. Position in orbital plane
    float x0 = planet->getSemiMajorAxis() * std::cos(theta);
    float z0 = planet->getSemiMinorAxis() * std::sin(theta);

    // 2. Inclination
    float x1 = x0;
    float y1 = z0 * std::sin(iRad);
    float z1 = z0 * std::cos(iRad);

    // 3. Longitude of ascending node
    glm::vec3 position;
    position.x = x1 * std::cos(omegaRad) - z1 * std::sin(omegaRad);
    position.y = y1;
    position.z = x1 * std::sin(omegaRad) + z1 * std::cos(omegaRad);

    return position;
}

Mesh Spacecraft::createUnitCone(unsigned int segments) {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    const float PI = 3.14159265359f;
    float halfHeight = 0.5f;
    float radius = 0.5f;

    // 1. Side wall vertices
    for (unsigned int i = 0; i < segments; ++i) {
        float angle1 = 2.0f * PI * i / segments;
        float angle2 = 2.0f * PI * (i + 1) / segments;
        
        float x1 = std::cos(angle1);
        float z1 = std::sin(angle1);
        float x2 = std::cos(angle2);
        float z2 = std::sin(angle2);
        
        // Apex vertex
        Vertex apex;
        apex.position = glm::vec3(0.0f, halfHeight, 0.0f);
        float midAngle = (angle1 + angle2) * 0.5f;
        apex.normal = glm::normalize(glm::vec3(std::cos(midAngle), 0.5f, std::sin(midAngle)));
        apex.color = glm::vec3(1.0f);
        apex.texCoords = glm::vec2(0.5f, 1.0f);
        unsigned int idxApex = vertices.size();
        vertices.push_back(apex);
        
        // Base left vertex
        Vertex baseL;
        baseL.position = glm::vec3(x1 * radius, -halfHeight, z1 * radius);
        baseL.normal = glm::normalize(glm::vec3(x1, 0.5f, z1));
        baseL.color = glm::vec3(1.0f);
        baseL.texCoords = glm::vec2((float)i / segments, 0.0f);
        unsigned int idxL = vertices.size();
        vertices.push_back(baseL);
        
        // Base right vertex
        Vertex baseR;
        baseR.position = glm::vec3(x2 * radius, -halfHeight, z2 * radius);
        baseR.normal = glm::normalize(glm::vec3(x2, 0.5f, z2));
        baseR.color = glm::vec3(1.0f);
        baseR.texCoords = glm::vec2((float)(i + 1) / segments, 0.0f);
        unsigned int idxR = vertices.size();
        vertices.push_back(baseR);
        
        indices.push_back(idxApex);
        indices.push_back(idxL);
        indices.push_back(idxR);
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

    return Mesh(vertices, indices);
}
