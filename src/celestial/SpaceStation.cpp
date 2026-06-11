#include "celestial/SpaceStation.h"
#include "celestial/Planet.h"
#include "core/Renderer.h"
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>
#include <cstdlib>

SpaceStation::SpaceStation(const std::string& name, float orbitRadius, float orbitSpeed, std::shared_ptr<Planet> parentPlanet)
    : SceneObject(name), m_orbitRadius(orbitRadius), m_orbitSpeed(orbitSpeed),
      m_parentPlanet(parentPlanet), m_orbitAngle(0.0f) {
    
    // Spread out the station's start position along the orbit
    m_orbitAngle = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX) * 2.0f * 3.1415926f;

    if (m_parentPlanet) {
        m_transform.setParent(&m_parentPlanet->getTransform());
    }

    // Initialize unit primitives
    m_cubeMesh = std::make_unique<Mesh>(createUnitCube());
    m_cylinderMesh = std::make_unique<Mesh>(createUnitCylinder(16));
}

void SpaceStation::update(float deltaTime) {
    m_orbitAngle += m_orbitSpeed * deltaTime;

    // Calculate position in the XZ plane relative to the parent planet
    glm::vec3 relativePos(
        std::cos(m_orbitAngle) * m_orbitRadius,
        0.0f,
        std::sin(m_orbitAngle) * m_orbitRadius
    );

    m_transform.setPosition(relativePos);

    // Give it a slow rotation to align its orientation over time
    m_transform.setRotation(glm::vec3(0.0f, m_orbitAngle * 0.5f, 0.0f));
}

void SpaceStation::render(Renderer& renderer) {
    if (!m_cubeMesh || !m_cylinderMesh) return;

    glm::mat4 modelBase = m_transform.getModelMatrix();

    const Shader& shader = renderer.getShader();
    shader.use();

    // Disable texture mapping to use color overrides
    shader.setBool("useTexture", false);
    shader.setBool("useColorOverride", true);
    shader.setInt("planetId", -1);

    // 1. Central body module (pressurized modules Destiny, Unity, Zarya, etc.)
    // Cylinder is along local Y-axis by default, we rotate it 90 deg around X to align along Z
    glm::mat4 centralModel = modelBase;
    centralModel = glm::rotate(centralModel, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    centralModel = glm::scale(centralModel, glm::vec3(0.012f, 0.040f, 0.012f)); // radius = 0.006, height = 0.040
    shader.setVec3("colorOverride", glm::vec3(0.70f, 0.72f, 0.75f)); // metallic grey
    renderer.renderWithLighting(*m_cylinderMesh, shader, centralModel);

    // 2. Truss structure (supports the solar panel wings)
    // Cylinder along local Y-axis rotated 90 deg around Z to align along X
    glm::mat4 trussModel = modelBase;
    trussModel = glm::rotate(trussModel, glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    trussModel = glm::scale(trussModel, glm::vec3(0.004f, 0.070f, 0.004f)); // radius = 0.002, height = 0.070
    shader.setVec3("colorOverride", glm::vec3(0.55f, 0.55f, 0.58f)); // darker grey truss
    renderer.renderWithLighting(*m_cylinderMesh, shader, trussModel);

    // 3. Solar Panel Wings (two long solar panels, dark blue, rotated slightly for realism)
    // Left Wing
    glm::mat4 leftWingModel = modelBase;
    leftWingModel = glm::translate(leftWingModel, glm::vec3(-0.0475f, 0.0f, 0.0f)); // offset from center along X
    leftWingModel = glm::rotate(leftWingModel, glm::radians(15.0f), glm::vec3(1.0f, 0.0f, 0.0f)); // tilt
    leftWingModel = glm::scale(leftWingModel, glm::vec3(0.025f, 0.001f, 0.012f)); // flat rectangle
    shader.setVec3("colorOverride", glm::vec3(0.05f, 0.18f, 0.42f)); // dark blue
    renderer.renderWithLighting(*m_cubeMesh, shader, leftWingModel);

    // Right Wing
    glm::mat4 rightWingModel = modelBase;
    rightWingModel = glm::translate(rightWingModel, glm::vec3(0.0475f, 0.0f, 0.0f));
    rightWingModel = glm::rotate(rightWingModel, glm::radians(15.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    rightWingModel = glm::scale(rightWingModel, glm::vec3(0.025f, 0.001f, 0.012f));
    shader.setVec3("colorOverride", glm::vec3(0.05f, 0.18f, 0.42f));
    renderer.renderWithLighting(*m_cubeMesh, shader, rightWingModel);

    // Solar wing connectors
    // Left connector
    glm::mat4 leftConn = modelBase;
    leftConn = glm::translate(leftConn, glm::vec3(-0.035f, 0.0f, 0.0f));
    leftConn = glm::rotate(leftConn, glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    leftConn = glm::scale(leftConn, glm::vec3(0.003f, 0.010f, 0.003f));
    shader.setVec3("colorOverride", glm::vec3(0.65f, 0.65f, 0.68f));
    renderer.renderWithLighting(*m_cylinderMesh, shader, leftConn);

    // Right connector
    glm::mat4 rightConn = modelBase;
    rightConn = glm::translate(rightConn, glm::vec3(0.035f, 0.0f, 0.0f));
    rightConn = glm::rotate(rightConn, glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    rightConn = glm::scale(rightConn, glm::vec3(0.003f, 0.010f, 0.003f));
    shader.setVec3("colorOverride", glm::vec3(0.65f, 0.65f, 0.68f));
    renderer.renderWithLighting(*m_cylinderMesh, shader, rightConn);

    // 4. Smaller side modules (pressurized laboratories Columbus, Kibo, etc.)
    // Module 1: Sticking out along +Y
    glm::mat4 mod1 = modelBase;
    mod1 = glm::translate(mod1, glm::vec3(0.0f, 0.0075f, 0.012f));
    mod1 = glm::scale(mod1, glm::vec3(0.009f, 0.015f, 0.009f));
    shader.setVec3("colorOverride", glm::vec3(0.70f, 0.72f, 0.75f));
    renderer.renderWithLighting(*m_cylinderMesh, shader, mod1);

    // Module 2: Sticking out along -Y
    glm::mat4 mod2 = modelBase;
    mod2 = glm::translate(mod2, glm::vec3(0.0f, -0.0075f, -0.012f));
    mod2 = glm::scale(mod2, glm::vec3(0.009f, 0.015f, 0.009f));
    shader.setVec3("colorOverride", glm::vec3(0.70f, 0.72f, 0.75f));
    renderer.renderWithLighting(*m_cylinderMesh, shader, mod2);

    // Module 3: Sticking out along -X
    glm::mat4 mod3 = modelBase;
    mod3 = glm::translate(mod3, glm::vec3(-0.012f, 0.0f, 0.0f));
    mod3 = glm::rotate(mod3, glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    mod3 = glm::scale(mod3, glm::vec3(0.008f, 0.012f, 0.008f));
    shader.setVec3("colorOverride", glm::vec3(0.68f, 0.70f, 0.73f));
    renderer.renderWithLighting(*m_cylinderMesh, shader, mod3);

    // 5. Antenna and Docking Arm (Canadarm2 robotic arm and communication array)
    // Docking arm Part A
    glm::mat4 armPartA = modelBase;
    armPartA = glm::translate(armPartA, glm::vec3(0.0f, -0.006f, 0.010f));
    armPartA = glm::rotate(armPartA, glm::radians(30.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    armPartA = glm::translate(armPartA, glm::vec3(0.0f, -0.006f, 0.0f));
    armPartA = glm::scale(armPartA, glm::vec3(0.002f, 0.012f, 0.002f));
    shader.setVec3("colorOverride", glm::vec3(0.85f, 0.85f, 0.85f)); // white arm
    renderer.renderWithLighting(*m_cylinderMesh, shader, armPartA);

    // Docking arm Part B
    glm::mat4 armPartB = modelBase;
    armPartB = glm::translate(armPartB, glm::vec3(0.0f, -0.006f, 0.010f));
    armPartB = glm::rotate(armPartB, glm::radians(30.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    armPartB = glm::translate(armPartB, glm::vec3(0.0f, -0.012f, 0.0f));
    armPartB = glm::rotate(armPartB, glm::radians(-60.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    armPartB = glm::translate(armPartB, glm::vec3(0.0f, -0.005f, 0.0f));
    armPartB = glm::scale(armPartB, glm::vec3(0.002f, 0.010f, 0.002f));
    shader.setVec3("colorOverride", glm::vec3(0.85f, 0.85f, 0.85f));
    renderer.renderWithLighting(*m_cylinderMesh, shader, armPartB);

    // Antenna Mast
    glm::mat4 antennaMast = modelBase;
    antennaMast = glm::translate(antennaMast, glm::vec3(0.0f, 0.006f + 0.0075f, -0.015f));
    antennaMast = glm::scale(antennaMast, glm::vec3(0.001f, 0.015f, 0.001f));
    shader.setVec3("colorOverride", glm::vec3(0.80f, 0.80f, 0.80f));
    renderer.renderWithLighting(*m_cylinderMesh, shader, antennaMast);

    // Antenna Dish
    glm::mat4 antennaDish = modelBase;
    antennaDish = glm::translate(antennaDish, glm::vec3(0.0f, 0.006f + 0.015f, -0.015f));
    antennaDish = glm::scale(antennaDish, glm::vec3(0.006f, 0.001f, 0.006f));
    shader.setVec3("colorOverride", glm::vec3(0.85f, 0.70f, 0.20f)); // golden dish
    renderer.renderWithLighting(*m_cylinderMesh, shader, antennaDish);

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

    // 1. Side wall vertices (double for sharp normals and textures)
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
