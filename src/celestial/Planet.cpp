#include "celestial/Planet.h"
#include "core/Renderer.h"
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>
#include <cstdlib>

Planet::Planet(const std::string& name, float radius, float orbitRadius, float orbitSpeed, float rotationSpeed, const glm::vec3& color, const std::string& texturePath)
    : SceneObject(name), m_orbitRadius(orbitRadius), m_orbitSpeed(orbitSpeed),
      m_rotationSpeed(rotationSpeed), m_color(color), m_orbitAngle(0.0f), m_rotationAngle(0.0f) {
    // Spread out planets by randomizing their initial orbit position
    m_orbitAngle = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX) * 2.0f * 3.1415926f;
    m_transform.setScale(glm::vec3(radius));

    if (!texturePath.empty()) {
        m_texture = std::make_unique<Texture>();
        if (!m_texture->load(texturePath)) {
            m_texture.reset(); // Falls back to color if load fails
        }
    }
}

void Planet::update(float deltaTime) {
    m_orbitAngle += m_orbitSpeed * deltaTime;
    m_rotationAngle += m_rotationSpeed * deltaTime;

    // Calculate 3D position in the XZ plane
    glm::vec3 position;
    position.x = std::cos(m_orbitAngle) * m_orbitRadius;
    position.y = 0.0f;
    position.z = std::sin(m_orbitAngle) * m_orbitRadius;

    m_transform.setPosition(position);
    m_transform.setRotation(glm::vec3(0.0f, m_rotationAngle, 0.0f));
}

void Planet::render(Renderer& renderer) {
    glm::mat4 model = m_transform.getModelMatrix();

    const Shader& shader = renderer.getShader();
    shader.use();

    bool hasTexture = m_texture && m_texture->isValid();
    shader.setBool("useTexture", hasTexture);
    if (hasTexture) {
        m_texture->bind(0);
        shader.setInt("planetTexture", 0);
    }

    shader.setBool("useColorOverride", true);
    shader.setVec3("colorOverride", m_color);

    int planetId = -1;
    if (m_name == "Mercury") planetId = 1;
    else if (m_name == "Venus") planetId = 2;
    else if (m_name == "Earth") planetId = 3;
    else if (m_name == "Mars") planetId = 4;
    else if (m_name == "Jupiter") planetId = 5;
    else if (m_name == "Saturn") planetId = 6;
    else if (m_name == "Uranus") planetId = 7;
    else if (m_name == "Neptune") planetId = 8;
    
    shader.setInt("planetId", planetId);

    // Planets are lit by the central star
    renderer.renderWithLighting(renderer.getSphereMesh(), shader, model);

    shader.setBool("useColorOverride", false);
    if (hasTexture) {
        shader.setBool("useTexture", false);
    }
}
