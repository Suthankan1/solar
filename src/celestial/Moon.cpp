#include "celestial/Moon.h"
#include "celestial/Planet.h"
#include "core/Renderer.h"
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>
#include <cstdlib>

Moon::Moon(const std::string& name, float radius, float orbitRadius, float orbitSpeed, float rotationSpeed, const glm::vec3& color, std::shared_ptr<Planet> parentPlanet, const std::string& texturePath)
    : SceneObject(name), m_orbitRadius(orbitRadius), m_orbitSpeed(orbitSpeed),
      m_rotationSpeed(rotationSpeed), m_color(color), m_parentPlanet(parentPlanet),
      m_orbitAngle(0.0f), m_rotationAngle(0.0f), m_worldPosition(0.0f) {
    m_orbitAngle = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX) * 2.0f * 3.1415926f;
    m_transform.setScale(glm::vec3(radius));

    if (!texturePath.empty()) {
        m_texture = std::make_unique<Texture>();
        if (!m_texture->load(texturePath)) {
            m_texture.reset(); // Falls back to color if load fails
        }
    }
}

void Moon::update(float deltaTime) {
    m_orbitAngle += m_orbitSpeed * deltaTime;
    m_rotationAngle += m_rotationSpeed * deltaTime;

    glm::vec3 parentPos = m_parentPlanet ? m_parentPlanet->getPosition() : glm::vec3(0.0f);
    float x = std::cos(m_orbitAngle) * m_orbitRadius;
    float z = std::sin(m_orbitAngle) * m_orbitRadius;
    glm::vec3 worldPos = parentPos + glm::vec3(x, 0.0f, z);

    m_transform.setPosition(worldPos);
    m_worldPosition = worldPos;
    m_transform.setRotation(glm::vec3(0.0f, m_rotationAngle, 0.0f));
}

void Moon::render(Renderer& renderer) {
    // 1. Camera-distance cull: skip tiny moons when far away
    float dist = glm::distance(renderer.getCameraPosition(), getPosition());
    float size = m_transform.getScale().x;
    if (dist > 15.0f && size < 0.15f) return;

    // 2. Frustum cull: skip objects outside the camera view frustum
    if (!renderer.sphereInFrustum(getPosition(), getRadius() * 2.0f))
        return;

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
    if (m_name == "Moon") planetId = 9;
    else if (m_name == "Phobos") planetId = 10;
    else if (m_name == "Io") planetId = 11;
    else if (m_name == "Europa") planetId = 12;

    shader.setInt("planetId", planetId);

    // Moons are lit by the central star
    renderer.renderWithLighting(renderer.getSphereMesh(), shader, model);

    shader.setBool("useColorOverride", false);
    shader.setBool("useTexture", false);
}
