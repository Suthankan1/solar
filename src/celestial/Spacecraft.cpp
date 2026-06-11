#include "celestial/Spacecraft.h"
#include "celestial/Planet.h"
#include "core/Renderer.h"
#include <glm/gtc/matrix_transform.hpp>
#include <GLFW/glfw3.h>
#include <cmath>
#include <cstdlib>

Spacecraft::Spacecraft(const std::string& name, std::shared_ptr<Planet> earth, std::shared_ptr<Planet> mars)
    : SceneObject(name), m_earth(earth), m_mars(mars), m_progress(0.0f), m_position(0.0f), m_forward(0.0f, 0.0f, 1.0f) {
    
    // Initialize unit primitives
    m_cubeMesh = std::make_unique<Mesh>(createUnitCube());
    m_cylinderMesh = std::make_unique<Mesh>(createUnitCylinder(16));

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

    glm::vec3 E = m_earth->getPosition();
    glm::vec3 M = m_mars->getPosition();

    // Orbit tangents (prograde velocity direction in the XZ plane)
    glm::vec3 tE = glm::normalize(glm::vec3(-E.z, 0.0f, E.x));
    glm::vec3 tM = glm::normalize(glm::vec3(-M.z, 0.0f, M.x));

    float d = glm::distance(E, M);
    float RE = m_earth->getRadius();
    float RM = m_mars->getRadius();

    // Shift endpoints just outside the planetary spheres to prevent clipping
    glm::vec3 P0 = E + tE * (RE + 0.02f);
    glm::vec3 P3 = M - tM * (RM + 0.02f);

    // Control points to shape the curved transfer orbit
    glm::vec3 P1 = E + 0.35f * d * tE;
    glm::vec3 P2 = M - 0.35f * d * tM;

    // Cubic Bezier interpolation
    float u = 1.0f - t;
    float tt = t * t;
    float uu = u * u;
    float uuu = uu * u;
    float ttt = tt * t;

    return uuu * P0 + 3.0f * uu * t * P1 + 3.0f * u * tt * P2 + ttt * P3;
}

glm::vec3 Spacecraft::getTangentForT(float t) const {
    if (!m_earth || !m_mars) return glm::vec3(0.0f, 0.0f, 1.0f);

    glm::vec3 E = m_earth->getPosition();
    glm::vec3 M = m_mars->getPosition();

    glm::vec3 tE = glm::normalize(glm::vec3(-E.z, 0.0f, E.x));
    glm::vec3 tM = glm::normalize(glm::vec3(-M.z, 0.0f, M.x));

    float d = glm::distance(E, M);
    float RE = m_earth->getRadius();
    float RM = m_mars->getRadius();

    glm::vec3 P0 = E + tE * (RE + 0.02f);
    glm::vec3 P3 = M - tM * (RM + 0.02f);

    glm::vec3 P1 = E + 0.35f * d * tE;
    glm::vec3 P2 = M - 0.35f * d * tM;

    float u = 1.0f - t;
    // B'(t) = 3*(1-t)^2*(P1-P0) + 6*(1-t)*t*(P2-P1) + 3*t^2*(P3-P2)
    glm::vec3 tangent = 3.0f * u * u * (P1 - P0) + 6.0f * u * t * (P2 - P1) + 3.0f * t * t * (P3 - P2);
    if (glm::length(tangent) > 0.0001f) {
        return glm::normalize(tangent);
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

    m_position = getPositionForT(m_progress);
    m_forward = getTangentForT(m_progress);

    m_transform.setPosition(m_position);

    // Calculate rotation to align local Z-axis (forward) with movement tangent
    float pitch = -std::asin(m_forward.y);
    float yaw = std::atan2(m_forward.x, m_forward.z);
    m_transform.setRotation(glm::vec3(pitch, yaw, 0.0f));

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
    if (!m_cubeMesh || !m_cylinderMesh || !m_trajectoryMesh) return;

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

    // 2a. Main Body (metallic white/silver cylinder aligned to local Z-axis)
    glm::mat4 bodyModel = modelBase;
    bodyModel = glm::rotate(bodyModel, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f)); // rotate local Y to Z
    bodyModel = glm::scale(bodyModel, glm::vec3(0.008f, 0.024f, 0.008f)); // r = 0.004, height = 0.024
    shader.setVec3("colorOverride", glm::vec3(0.88f, 0.90f, 0.92f)); // silver/white
    renderer.renderWithLighting(*m_cylinderMesh, shader, bodyModel);

    // 2b. Cockpit Window (dark tinted glass blue/cyan cube)
    glm::mat4 windowModel = modelBase;
    windowModel = glm::translate(windowModel, glm::vec3(0.0f, 0.0035f, 0.006f)); // offset up and forward
    windowModel = glm::scale(windowModel, glm::vec3(0.006f, 0.0025f, 0.005f));
    shader.setVec3("colorOverride", glm::vec3(0.02f, 0.55f, 0.78f)); // glossy cyan-blue
    renderer.renderWithLighting(*m_cubeMesh, shader, windowModel);

    // 2c. Left and Right Solar Panels (dark blue rectangles with grey masts)
    // Left Mast
    glm::mat4 leftMastModel = modelBase;
    leftMastModel = glm::translate(leftMastModel, glm::vec3(-0.007f, 0.0f, 0.0f));
    leftMastModel = glm::rotate(leftMastModel, glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f)); // align along X
    leftMastModel = glm::scale(leftMastModel, glm::vec3(0.001f, 0.006f, 0.001f));
    shader.setVec3("colorOverride", glm::vec3(0.55f, 0.55f, 0.58f));
    renderer.renderWithLighting(*m_cylinderMesh, shader, leftMastModel);

    // Left Panel
    glm::mat4 leftPanelModel = modelBase;
    leftPanelModel = glm::translate(leftPanelModel, glm::vec3(-0.019f, 0.0f, 0.0f));
    leftPanelModel = glm::scale(leftPanelModel, glm::vec3(0.018f, 0.0006f, 0.009f));
    shader.setVec3("colorOverride", glm::vec3(0.04f, 0.16f, 0.38f)); // deep solar panel blue
    renderer.renderWithLighting(*m_cubeMesh, shader, leftPanelModel);

    // Right Mast
    glm::mat4 rightMastModel = modelBase;
    rightMastModel = glm::translate(rightMastModel, glm::vec3(0.007f, 0.0f, 0.0f));
    rightMastModel = glm::rotate(rightMastModel, glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    rightMastModel = glm::scale(rightMastModel, glm::vec3(0.001f, 0.006f, 0.001f));
    shader.setVec3("colorOverride", glm::vec3(0.55f, 0.55f, 0.58f));
    renderer.renderWithLighting(*m_cylinderMesh, shader, rightMastModel);

    // Right Panel
    glm::mat4 rightPanelModel = modelBase;
    rightPanelModel = glm::translate(rightPanelModel, glm::vec3(0.019f, 0.0f, 0.0f));
    rightPanelModel = glm::scale(rightPanelModel, glm::vec3(0.018f, 0.0006f, 0.009f));
    shader.setVec3("colorOverride", glm::vec3(0.04f, 0.16f, 0.38f));
    renderer.renderWithLighting(*m_cubeMesh, shader, rightPanelModel);

    // 2d. Engine Nozzle (dark metal cylinder at the rear)
    glm::mat4 nozzleModel = modelBase;
    nozzleModel = glm::translate(nozzleModel, glm::vec3(0.0f, 0.0f, -0.013f));
    nozzleModel = glm::rotate(nozzleModel, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    nozzleModel = glm::scale(nozzleModel, glm::vec3(0.006f, 0.004f, 0.006f));
    shader.setVec3("colorOverride", glm::vec3(0.25f, 0.25f, 0.28f)); // dark carbon metallic
    renderer.renderWithLighting(*m_cylinderMesh, shader, nozzleModel);

    // 2e. Small Glowing Exhaust Trail (flickering orange flame, unlit, semi-transparent)
    float timeVal = (float)glfwGetTime();
    float flicker = 1.0f + 0.12f * std::sin(timeVal * 26.0f);
    float lengthMod = 1.0f + 0.08f * std::cos(timeVal * 19.0f);

    glm::mat4 exhaustModel = modelBase;
    exhaustModel = glm::translate(exhaustModel, glm::vec3(0.0f, 0.0f, -0.021f)); // extend backwards from nozzle
    exhaustModel = glm::rotate(exhaustModel, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    exhaustModel = glm::scale(exhaustModel, glm::vec3(0.0028f * flicker, 0.012f * lengthMod, 0.0028f * flicker));

    shader.setVec3("colorOverride", glm::vec3(1.0f, 0.42f, 0.05f)); // brilliant fiery orange
    shader.setFloat("emissiveStrength", 2.2f);
    shader.setFloat("globalAlpha", 0.75f);
    
    // Draw exhaust trail without lighting so it stays perfectly bright
    renderer.render(*m_cylinderMesh, shader, exhaustModel);

    // Clean up shader states
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
