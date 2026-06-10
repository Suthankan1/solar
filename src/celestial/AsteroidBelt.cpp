#include "celestial/AsteroidBelt.h"
#include "core/Renderer.h"
#include "core/Shader.h"
#include <cstdlib>
#include <cmath>

AsteroidBelt::AsteroidBelt(const std::string& name, float innerRadius, float outerRadius, unsigned int count)
    : SceneObject(name), m_mesh(nullptr), m_angle(0.0f) {
    
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    const float PI = 3.14159265359f;

    for (unsigned int i = 0; i < count; ++i) {
        float r_rand = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
        float angle_rand = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
        float y_rand = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);

        float r = innerRadius + r_rand * (outerRadius - innerRadius);
        float angle = angle_rand * 2.0f * PI;
        float yOffset = (y_rand - 0.5f) * 0.3f; // slight vertical spread

        float x = std::cos(angle) * r;
        float y = yOffset;
        float z = std::sin(angle) * r;

        Vertex v;
        v.position = glm::vec3(x, y, z);
        v.normal = glm::normalize(v.position);
        
        // Random grey-brown: glm::vec3(0.4+rand*0.2, 0.35+rand*0.15, 0.3+rand*0.1)
        float c_rand_r = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
        float c_rand_g = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
        float c_rand_b = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
        v.color = glm::vec3(
            0.4f + c_rand_r * 0.2f,
            0.35f + c_rand_g * 0.15f,
            0.3f + c_rand_b * 0.1f
        );
        v.texCoords = glm::vec2(0.0f);

        vertices.push_back(v);
        indices.push_back(i);
    }

    m_mesh = std::make_unique<Mesh>(vertices, indices, GL_POINTS);
}

void AsteroidBelt::update(float deltaTime) {
    m_angle += 0.02f * deltaTime;
    m_transform.setRotation(glm::vec3(0.0f, m_angle, 0.0f));
}

void AsteroidBelt::render(Renderer& renderer) {
    if (!m_mesh) return;

    glPointSize(1.5f);
    const Shader& shader = renderer.getShader();
    shader.use();
    shader.setBool("useColorOverride", false);
    renderer.render(*m_mesh, shader, m_transform.getModelMatrix());
    glPointSize(1.0f);
}
