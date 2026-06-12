#include "celestial/Spacecraft.h"
#include "celestial/Planet.h"
#include "core/Renderer.h"
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>

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

    auto renderCube = [&](const glm::mat4& parent, const glm::vec3& offset, const glm::vec3& scale,
                          const glm::vec3& rotation, const glm::vec3& color) {
        glm::mat4 model = parent;
        model = glm::translate(model, offset);
        model = glm::rotate(model, rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
        model = glm::rotate(model, rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::rotate(model, rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
        model = glm::scale(model, scale);
        shader.setVec3("colorOverride", color);
        renderer.renderWithLighting(*m_cubeMesh, shader, model);
    };

    auto renderCylinder = [&](const glm::mat4& parent, const glm::vec3& offset, const glm::vec3& scale,
                              const glm::vec3& rotation, const glm::vec3& color) {
        glm::mat4 model = parent;
        model = glm::translate(model, offset);
        model = glm::rotate(model, rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
        model = glm::rotate(model, rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::rotate(model, rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
        model = glm::scale(model, scale);
        shader.setVec3("colorOverride", color);
        renderer.renderWithLighting(*m_cylinderMesh, shader, model);
    };

    auto renderCone = [&](const glm::mat4& parent, const glm::vec3& offset, const glm::vec3& scale,
                          const glm::vec3& rotation, const glm::vec3& color) {
        glm::mat4 model = parent;
        model = glm::translate(model, offset);
        model = glm::rotate(model, rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
        model = glm::rotate(model, rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::rotate(model, rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
        model = glm::scale(model, scale);
        shader.setVec3("colorOverride", color);
        renderer.renderWithLighting(*m_coneMesh, shader, model);
    };

    const glm::vec3 whiteMod(0.72f, 0.72f, 0.74f);
    const glm::vec3 darkBlue(0.05f, 0.12f, 0.40f);
    const glm::vec3 metal(0.82f, 0.82f, 0.85f);
    const glm::vec3 goldFoil(0.95f, 0.68f, 0.18f);
    const glm::vec3 thrusterGrey(0.55f, 0.50f, 0.45f);

    // Spacecraft bus body.
    renderCube(modelBase, glm::vec3(0.0f), glm::vec3(0.020f, 0.018f, 0.025f), glm::vec3(0.0f), whiteMod);

    // High-gain antenna dish and feed, facing the forward/Earth-pointing side of the probe.
    renderCylinder(modelBase, glm::vec3(0.0f, 0.0f, 0.016f), glm::vec3(0.015f, 0.002f, 0.015f),
                   glm::vec3(glm::radians(90.0f), 0.0f, 0.0f), metal);
    renderCylinder(modelBase, glm::vec3(0.0f, 0.0f, 0.024f), glm::vec3(0.0012f, 0.010f, 0.0012f),
                   glm::vec3(glm::radians(90.0f), 0.0f, 0.0f), metal);
    renderCylinder(modelBase, glm::vec3(0.0f, 0.0f, 0.030f), glm::vec3(0.0022f, 0.0015f, 0.0022f),
                   glm::vec3(glm::radians(90.0f), 0.0f, 0.0f), metal);

    // Solar panel wings with slim support booms.
    renderCylinder(modelBase, glm::vec3(-0.020f, 0.0f, 0.0f), glm::vec3(0.0008f, 0.020f, 0.0008f),
                   glm::vec3(0.0f, 0.0f, glm::radians(90.0f)), metal);
    renderCylinder(modelBase, glm::vec3(0.020f, 0.0f, 0.0f), glm::vec3(0.0008f, 0.020f, 0.0008f),
                   glm::vec3(0.0f, 0.0f, glm::radians(90.0f)), metal);
    renderCube(modelBase, glm::vec3(-0.030f, 0.0f, 0.0f), glm::vec3(0.028f, 0.0003f, 0.010f), glm::vec3(0.0f), darkBlue);
    renderCube(modelBase, glm::vec3(0.030f, 0.0f, 0.0f), glm::vec3(0.028f, 0.0003f, 0.010f), glm::vec3(0.0f), darkBlue);

    // Subtle solar-cell grid strips.
    renderCube(modelBase, glm::vec3(-0.030f, 0.00025f, -0.0025f), glm::vec3(0.027f, 0.0002f, 0.0004f), glm::vec3(0.0f), glm::vec3(0.18f, 0.28f, 0.58f));
    renderCube(modelBase, glm::vec3(-0.030f, 0.00025f, 0.0025f), glm::vec3(0.027f, 0.0002f, 0.0004f), glm::vec3(0.0f), glm::vec3(0.18f, 0.28f, 0.58f));
    renderCube(modelBase, glm::vec3(0.030f, 0.00025f, -0.0025f), glm::vec3(0.027f, 0.0002f, 0.0004f), glm::vec3(0.0f), glm::vec3(0.18f, 0.28f, 0.58f));
    renderCube(modelBase, glm::vec3(0.030f, 0.00025f, 0.0025f), glm::vec3(0.027f, 0.0002f, 0.0004f), glm::vec3(0.0f), glm::vec3(0.18f, 0.28f, 0.58f));

    // Aft thruster nozzle: a short neck and flared bell.
    renderCylinder(modelBase, glm::vec3(0.0f, 0.0f, -0.016f), glm::vec3(0.006f, 0.008f, 0.006f),
                   glm::vec3(glm::radians(90.0f), 0.0f, 0.0f), thrusterGrey);
    renderCone(modelBase, glm::vec3(0.0f, 0.0f, -0.022f), glm::vec3(0.010f, 0.010f, 0.010f),
               glm::vec3(glm::radians(-90.0f), 0.0f, 0.0f), thrusterGrey);

    // Animated ion exhaust plume behind the aft thruster.
    float plumeScale = 0.8f + 0.2f * std::sin(static_cast<float>(glfwGetTime()) * 12.0f);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    shader.setFloat("emissiveStrength", 1.2f);
    shader.setFloat("globalAlpha", 0.7f * plumeScale);
    shader.setVec3("colorOverride", glm::vec3(0.5f, 0.8f, 1.0f));

    glm::mat4 plumeMat = glm::translate(modelBase, glm::vec3(0.0f, 0.0f, -0.022f));
    plumeMat = glm::rotate(plumeMat, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    plumeMat = glm::scale(plumeMat, glm::vec3(0.004f, 0.008f * plumeScale, 0.004f));
    renderer.renderWithLighting(*m_coneMesh, shader, plumeMat);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    shader.setFloat("emissiveStrength", 0.0f);
    shader.setFloat("globalAlpha", 1.0f);

    // Thermal blanket gold foil patches on the bus sides.
    renderCube(modelBase, glm::vec3(0.0f, 0.0094f, 0.001f), glm::vec3(0.015f, 0.0008f, 0.018f), glm::vec3(0.0f), goldFoil);
    renderCube(modelBase, glm::vec3(0.0f, -0.0094f, -0.002f), glm::vec3(0.013f, 0.0008f, 0.016f), glm::vec3(0.0f), goldFoil);
    renderCube(modelBase, glm::vec3(-0.0104f, 0.0f, -0.002f), glm::vec3(0.0008f, 0.012f, 0.014f), glm::vec3(0.0f), goldFoil);
    renderCube(modelBase, glm::vec3(0.0104f, 0.0f, 0.002f), glm::vec3(0.0008f, 0.012f, 0.014f), glm::vec3(0.0f), goldFoil);

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
