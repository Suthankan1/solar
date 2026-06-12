#include "celestial/AsteroidBelt.h"
#include "core/Renderer.h"
#include "core/Shader.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <cstdlib>
#include <cmath>
#include <vector>
#include <memory>
#include <glm/gtc/matrix_transform.hpp>

AsteroidBelt::AsteroidBelt(const std::string& name, float innerRadius, float outerRadius, unsigned int count)
    : SceneObject(name), m_count(kAsteroidCount) {
    (void)count;
    
    // 1. Compile the custom instanced asteroid shader using existing cube.frag
    try {
        m_shader = std::make_unique<Shader>("shaders/asteroid.vert", "shaders/cube.frag");
    } catch (const std::exception& e) {
        // Fallback to standard shader if custom compile fails
        m_shader = nullptr;
    }

    // 2. Generate 5 unique rocky asteroid meshes to provide shape variation
    m_baseMeshes.reserve(5);
    m_baseMeshes.push_back(createRockyAsteroid(1.0f, 0.20f, 100));
    m_baseMeshes.push_back(createRockyAsteroid(1.0f, 0.25f, 200));
    m_baseMeshes.push_back(createRockyAsteroid(1.0f, 0.30f, 300));
    m_baseMeshes.push_back(createRockyAsteroid(1.0f, 0.35f, 400));
    m_baseMeshes.push_back(createRockyAsteroid(1.0f, 0.40f, 500));

    // 3. Setup instanced vertex attributes and buffers on each mesh's VAO
    m_instanceVBOs.resize(m_baseMeshes.size());
    m_instanceDataPerMesh.resize(m_baseMeshes.size());

    for (size_t i = 0; i < m_baseMeshes.size(); ++i) {
        glGenBuffers(1, &m_instanceVBOs[i]);
        glBindBuffer(GL_ARRAY_BUFFER, m_instanceVBOs[i]);
        // Pre-allocate GPU buffer to support up to the maximum possible count
        glBufferData(GL_ARRAY_BUFFER, m_count * sizeof(InstanceData), nullptr, GL_DYNAMIC_DRAW);

        glBindVertexArray(m_baseMeshes[i].VAO);

        GLsizei stride = sizeof(InstanceData);

        // Instance Model Matrix: layout location 4, 5, 6, 7 (consumes 4 vec4s)
        glEnableVertexAttribArray(4);
        glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(InstanceData, modelMatrix));

        glEnableVertexAttribArray(5);
        glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, stride, (void*)(offsetof(InstanceData, modelMatrix) + sizeof(glm::vec4)));

        glEnableVertexAttribArray(6);
        glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, stride, (void*)(offsetof(InstanceData, modelMatrix) + 2 * sizeof(glm::vec4)));

        glEnableVertexAttribArray(7);
        glVertexAttribPointer(7, 4, GL_FLOAT, GL_FALSE, stride, (void*)(offsetof(InstanceData, modelMatrix) + 3 * sizeof(glm::vec4)));

        // Instance Color: layout location 8 (vec3)
        glEnableVertexAttribArray(8);
        glVertexAttribPointer(8, 3, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(InstanceData, color));

        // Configure as instanced attributes (update once per instance, not per vertex)
        glVertexAttribDivisor(4, 1);
        glVertexAttribDivisor(5, 1);
        glVertexAttribDivisor(6, 1);
        glVertexAttribDivisor(7, 1);
        glVertexAttribDivisor(8, 1);

        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    // 4. Distribute the asteroids with realistic variations
    m_asteroids.reserve(m_count);
    m_speedMultipliers.reserve(m_count);
    m_angles.reserve(m_count);
    const float PI = 3.14159265359f;
    const float heroRadii[] = {5.8f, 6.1f, 6.4f};
    const float heroInitialAngles[] = {0.35f * PI, 1.15f * PI, 1.75f * PI};

    auto rand01 = []() {
        return static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
    };

    m_heroMesh = std::make_unique<Mesh>(Renderer::createSphere(0.04f, 8, 8));
    m_heroPositions.reserve(3);
    m_heroAngles.reserve(3);
    for (int i = 0; i < 3; ++i) {
        m_heroAngles.push_back(heroInitialAngles[i]);
        m_heroPositions.emplace_back(
            std::cos(heroInitialAngles[i]) * heroRadii[i],
            0.0f,
            std::sin(heroInitialAngles[i]) * heroRadii[i]
        );
    }

    for (unsigned int i = 0; i < m_count; ++i) {
        Asteroid a;

        // Radial distribution
        float r_rand = rand01();
        a.radius = innerRadius + r_rand * (outerRadius - innerRadius);

        // Keplerian orbital speed (slower outer orbit, v proportional to 1/sqrt(r))
        // Mars orbit speed in main.cpp is 0.7 at radius 5.0
        float baseSpeed = 0.7f * std::sqrt(5.0f / a.radius);
        a.orbitSpeed = baseSpeed;
        m_speedMultipliers.push_back(0.85f + rand01() * 0.30f);

        // Random initial orbit angle
        a.orbitAngle = rand01() * 2.0f * PI;
        m_angles.push_back(a.orbitAngle);

        // Individual self-rotation parameters
        a.rotationAngle = rand01() * 2.0f * PI;
        a.rotationSpeed = 0.3f + rand01() * 1.5f;

        float rx = rand01() - 0.5f;
        float ry = rand01() - 0.5f;
        float rz = rand01() - 0.5f;
        a.rotationAxis = glm::normalize(glm::vec3(rx, ry, rz));

        // Size tiers: 600 tiny, 160 medium, 40 large.
        float pointSize = 0.0f;
        float verticalSpread = 0.0f;
        if (i < kTinyAsteroidCount) {
            pointSize = 1.0f + rand01() * 0.8f;
            verticalSpread = 0.40f;
        } else if (i < kTinyAsteroidCount + kMediumAsteroidCount) {
            pointSize = 2.5f + rand01() * 1.5f;
            verticalSpread = 0.25f;
        } else {
            pointSize = 5.0f + rand01() * 3.0f;
            verticalSpread = 0.15f;
        }
        a.yOffset = (rand01() - 0.5f) * verticalSpread;

        // Convert the requested point-size tiers into instanced mesh scale.
        float baseScale = pointSize * 0.015f;

        // Apply non-uniform scaling on X, Y, Z for irregular rocky dimensions
        float sx = baseScale * (0.8f + rand01() * 0.4f);
        float sy = baseScale * (0.8f + rand01() * 0.4f);
        float sz = baseScale * (0.8f + rand01() * 0.4f);
        a.scale = glm::vec3(sx, sy, sz);

        // Warm gray, dark gray, and reddish color mix.
        float colorType = rand01();
        if (colorType < 0.60f) {
            a.color = glm::vec3(0.65f, 0.60f, 0.55f);
        } else if (colorType < 0.90f) {
            a.color = glm::vec3(0.40f, 0.37f, 0.33f);
        } else {
            a.color = glm::vec3(0.55f, 0.42f, 0.35f);
        }

        // Assign to one of the 5 base meshes randomly
        a.meshIndex = std::rand() % m_baseMeshes.size();

        m_asteroids.push_back(a);
    }
}

AsteroidBelt::~AsteroidBelt() {
    if (!m_instanceVBOs.empty()) {
        glDeleteBuffers(static_cast<GLsizei>(m_instanceVBOs.size()), m_instanceVBOs.data());
    }
}

Mesh AsteroidBelt::createRockyAsteroid(float baseRadius, float roughness, int seed) {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    const float PI = 3.14159265359f;
    unsigned int rings = 10;
    unsigned int sectors = 10;

    // Locally-seeded random helper for consistent shapes
    struct ShapeRNG {
        unsigned int state;
        ShapeRNG(unsigned int s) : state(s) {}
        float nextFloat() {
            state = state * 1664525 + 1013904223;
            return static_cast<float>(state) / 4294967296.0f;
        }
    };
    ShapeRNG rng(seed);

    // Compute coordinate displacements (ensuring continuous poles and seams)
    std::vector<std::vector<float>> displacements(rings, std::vector<float>(sectors));
    float northPoleDisp = 1.0f + (rng.nextFloat() - 0.5f) * roughness;
    float southPoleDisp = 1.0f + (rng.nextFloat() - 0.5f) * roughness;

    for (unsigned int r = 0; r < rings; ++r) {
        for (unsigned int s = 0; s < sectors; ++s) {
            if (r == 0) {
                displacements[r][s] = northPoleDisp;
            } else if (r == rings - 1) {
                displacements[r][s] = southPoleDisp;
            } else if (s == 0 || s == sectors - 1) {
                if (s == 0) {
                    displacements[r][s] = 1.0f + (rng.nextFloat() - 0.5f) * roughness;
                } else {
                    displacements[r][s] = displacements[r][0]; // wrap seam
                }
            } else {
                displacements[r][s] = 1.0f + (rng.nextFloat() - 0.5f) * roughness;
            }
        }
    }

    // Create vertices
    for (unsigned int r = 0; r < rings; ++r) {
        float phi = PI * (float)r / (float)(rings - 1);
        for (unsigned int s = 0; s < sectors; ++s) {
            float theta = 2.0f * PI * (float)s / (float)(sectors - 1);

            float x = std::cos(theta) * std::sin(phi);
            float y = std::cos(phi);
            float z = std::sin(theta) * std::sin(phi);

            float disp = displacements[r][s];

            Vertex vertex;
            vertex.position = glm::vec3(x, y, z) * baseRadius * disp;
            vertex.normal = glm::normalize(vertex.position);
            vertex.color = glm::vec3(1.0f);
            vertex.texCoords = glm::vec2((float)s / (float)(sectors - 1), 1.0f - (float)r / (float)(rings - 1));

            vertices.push_back(vertex);
        }
    }

    // Create indices
    for (unsigned int r = 0; r < rings - 1; ++r) {
        for (unsigned int s = 0; s < sectors - 1; ++s) {
            unsigned int first = r * sectors + s;
            unsigned int second = first + 1;
            unsigned int third = (r + 1) * sectors + s;
            unsigned int fourth = third + 1;

            indices.push_back(first);
            indices.push_back(third);
            indices.push_back(second);

            indices.push_back(second);
            indices.push_back(third);
            indices.push_back(fourth);
        }
    }

    // Recompute normals for faceted, rocky lighting
    for (auto& v : vertices) {
        v.normal = glm::vec3(0.0f);
    }

    for (size_t i = 0; i < indices.size(); i += 3) {
        unsigned int idx0 = indices[i];
        unsigned int idx1 = indices[i + 1];
        unsigned int idx2 = indices[i + 2];

        glm::vec3 v0 = vertices[idx0].position;
        glm::vec3 v1 = vertices[idx1].position;
        glm::vec3 v2 = vertices[idx2].position;

        glm::vec3 edge1 = v1 - v0;
        glm::vec3 edge2 = v2 - v0;
        glm::vec3 normal = glm::cross(edge1, edge2);

        vertices[idx0].normal += normal;
        vertices[idx1].normal += normal;
        vertices[idx2].normal += normal;
    }

    for (auto& v : vertices) {
        if (glm::length(v.normal) > 0.0001f) {
            v.normal = glm::normalize(v.normal);
        } else {
            v.normal = glm::normalize(v.position);
        }
    }

    return Mesh(vertices, indices);
}

void AsteroidBelt::update(float deltaTime) {
    // Slowly rotate the entire belt system around the sun
    m_angle += 0.015f * deltaTime;
    m_transform.setRotation(glm::vec3(0.0f, m_angle, 0.0f));

    // Clear instance vectors
    for (auto& vec : m_instanceDataPerMesh) {
        vec.clear();
    }

    // Update individual physics states and build model matrices
    for (size_t i = 0; i < m_asteroids.size(); ++i) {
        auto& asteroid = m_asteroids[i];

        // Orbit revolution
        m_angles[i] += asteroid.orbitSpeed * m_speedMultipliers[i] * deltaTime;
        asteroid.orbitAngle = m_angles[i];
        
        // Self-rotation
        asteroid.rotationAngle += asteroid.rotationSpeed * deltaTime;

        // Construct local model matrix
        glm::mat4 localModel(1.0f);
        float x = std::cos(asteroid.orbitAngle) * asteroid.radius;
        float z = std::sin(asteroid.orbitAngle) * asteroid.radius;
        localModel = glm::translate(localModel, glm::vec3(x, asteroid.yOffset, z));
        localModel = glm::rotate(localModel, asteroid.rotationAngle, asteroid.rotationAxis);
        localModel = glm::scale(localModel, asteroid.scale);

        InstanceData data;
        data.modelMatrix = localModel;
        data.color = asteroid.color;

        m_instanceDataPerMesh[asteroid.meshIndex].push_back(data);
    }

    const float heroRadii[] = {5.8f, 6.1f, 6.4f};
    for (size_t i = 0; i < m_heroPositions.size(); ++i) {
        const float heroSpeed = 0.7f * std::sqrt(5.0f / heroRadii[i]) * 0.6f;
        m_heroAngles[i] += heroSpeed * deltaTime;
        m_heroPositions[i] = glm::vec3(
            std::cos(m_heroAngles[i]) * heroRadii[i],
            0.0f,
            std::sin(m_heroAngles[i]) * heroRadii[i]
        );
    }

    // Upload the updated matrices and colors to GPU VBOs
    for (size_t i = 0; i < m_baseMeshes.size(); ++i) {
        const auto& data = m_instanceDataPerMesh[i];
        if (data.empty()) continue;

        glBindBuffer(GL_ARRAY_BUFFER, m_instanceVBOs[i]);
        glBufferSubData(GL_ARRAY_BUFFER, 0, data.size() * sizeof(InstanceData), data.data());
    }
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void AsteroidBelt::render(Renderer& renderer) {
    // 1. Determine which shader program to use
    // Fall back to renderer's default shader if custom compile failed
    const Shader& shader = m_shader ? *m_shader : renderer.getShader();
    shader.use();

    // 2. Set projection, view, and lighting uniforms using current Renderer state
    shader.setMat4("model", m_transform.getModelMatrix());
    shader.setMat4("view", renderer.getViewMatrix());
    shader.setMat4("projection", renderer.getProjectionMatrix());

    shader.setBool("useLighting", true);
    shader.setVec3("lightPos", renderer.getLightPosition());
    shader.setVec3("lightColor", renderer.getLightColor());
    shader.setVec3("viewPos", renderer.getCameraPosition());
    shader.setFloat("lightConstant", renderer.getLightConstant());
    shader.setFloat("lightLinear", renderer.getLightLinear());
    shader.setFloat("lightQuadratic", renderer.getLightQuadratic());
    
    // Ensure base instanced colors from vertex inputs are preferred, not overriden
    shader.setBool("useColorOverride", false);
    shader.setFloat("globalAlpha", 1.0f);
    shader.setBool("useTexture", false);
    shader.setFloat("time", (float)glfwGetTime());
    shader.setInt("planetId", -1);
    shader.setFloat("pointSizeScale", 1.0f);
    shader.setBool("isAsteroidPointSprite", false);

    // 3. Render all meshes using instanced draw calls
    for (size_t i = 0; i < m_baseMeshes.size(); ++i) {
        const auto& data = m_instanceDataPerMesh[i];
        if (data.empty()) continue;

        glBindVertexArray(m_baseMeshes[i].VAO);
        shader.setBool("isAsteroidPointSprite", m_baseMeshes[i].drawMode == GL_POINTS);
        glDrawElementsInstanced(
            m_baseMeshes[i].drawMode,
            static_cast<GLsizei>(m_baseMeshes[i].indices.size()),
            GL_UNSIGNED_INT,
            nullptr,
            static_cast<GLsizei>(data.size())
        );
    }
    glBindVertexArray(0);

    if (m_heroMesh) {
        shader.setBool("useColorOverride", true);
        shader.setVec3("colorOverride", glm::vec3(0.50f, 0.46f, 0.40f));
        shader.setBool("isAsteroidPointSprite", false);

        for (size_t i = 0; i < m_heroPositions.size(); ++i) {
            glm::mat4 heroModel = m_transform.getModelMatrix();
            heroModel = glm::translate(heroModel, m_heroPositions[i]);
            renderer.renderWithLighting(*m_heroMesh, shader, heroModel);
        }
    }

    shader.setBool("useColorOverride", false);
    shader.setFloat("globalAlpha", 1.0f);
    shader.setBool("useTexture", false);
    shader.setInt("planetId", -1);
    shader.setBool("isAsteroidPointSprite", false);
}
