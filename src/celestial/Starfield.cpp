#include "celestial/Starfield.h"
#include "core/Renderer.h"
#include <glm/gtc/matrix_transform.hpp>
#include <cstdlib>
#include <cmath>

Starfield::Starfield(const std::string& name, unsigned int count, float radius)
    : SceneObject(name), m_mesh(nullptr) {
    
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    const float PI = 3.14159265359f;

    for (unsigned int i = 0; i < count; ++i) {
        // Generate uniform points on a sphere
        float theta = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX) * 2.0f * PI;
        float phi = std::acos(2.0f * (static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX)) - 1.0f);

        float x = std::cos(theta) * std::sin(phi) * radius;
        float y = std::cos(phi) * radius;
        float z = std::sin(theta) * std::sin(phi) * radius;

        Vertex v;
        v.position = glm::vec3(x, y, z);
        v.normal = -glm::normalize(v.position);
        
        // Random brightness for shimmering star aesthetic
        float brightness = 0.6f + static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX) * 0.4f;
        v.color = glm::vec3(brightness);
        v.texCoords = glm::vec2(0.0f);

        vertices.push_back(v);
        indices.push_back(i);
    }

    m_mesh = std::make_unique<Mesh>(vertices, indices, GL_POINTS);
}

void Starfield::render(Renderer& renderer) {
    if (!m_mesh) return;

    // Center the starfield around the camera position to make it feel infinitely far away
    glm::vec3 cameraPos = renderer.getCameraPosition();
    
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, cameraPos);

    const Shader& shader = renderer.getShader();
    shader.use();
    shader.setBool("useColorOverride", false);

    // Disable depth write so stars always render behind everything else
    glDepthMask(GL_FALSE);

    renderer.render(*m_mesh, shader, model);

    // Re-enable depth write
    glDepthMask(GL_TRUE);
}
