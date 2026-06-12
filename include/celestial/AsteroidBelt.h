#pragma once

#include "scene/SceneObject.h"
#include "scene/Transform.h"
#include "core/Mesh.h"
#include "core/Shader.h"
#include <memory>
#include <string>
#include <vector>
#include <glm/glm.hpp>

class AsteroidBelt : public SceneObject {
public:
    AsteroidBelt(const std::string& name, float innerRadius, float outerRadius, unsigned int count);
    virtual ~AsteroidBelt() override;

    void update(float deltaTime) override;
    void render(Renderer& renderer) override;

private:
    static constexpr unsigned int kAsteroidCount = 800;
    static constexpr unsigned int kTinyAsteroidCount = 600;
    static constexpr unsigned int kMediumAsteroidCount = 160;

    struct Asteroid {
        float radius;
        float orbitAngle;
        float orbitSpeed;
        float yOffset;
        float rotationAngle;
        float rotationSpeed;
        glm::vec3 rotationAxis;
        glm::vec3 scale;
        glm::vec3 color;
        unsigned int meshIndex;
    };

    struct InstanceData {
        glm::mat4 modelMatrix;
        glm::vec3 color;
    };

    // Helper to generate a unique rocky asteroid mesh shape
    Mesh createRockyAsteroid(float baseRadius, float roughness, int seed);

    unsigned int m_count;
    Transform m_transform;
    float m_angle = 0.0f;

    std::vector<Mesh> m_baseMeshes;
    std::vector<unsigned int> m_instanceVBOs;
    std::unique_ptr<Shader> m_shader;
    
    std::vector<Asteroid> m_asteroids;
    std::vector<float> m_speedMultipliers;
    std::vector<float> m_angles;
    std::vector<std::vector<InstanceData>> m_instanceDataPerMesh;
    std::vector<glm::vec3> m_heroPositions;
    std::vector<float> m_heroAngles;
    std::unique_ptr<Mesh> m_heroMesh;
};
