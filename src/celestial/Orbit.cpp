#include "celestial/Orbit.h"
#include "celestial/Planet.h"
#include "core/Renderer.h"
#include "core/Shader.h"
#include <glm/gtc/matrix_transform.hpp>

Orbit::Orbit(const std::string& name, float radius, const glm::vec3& color, std::shared_ptr<Planet> parentPlanet)
    : SceneObject(name), m_radius(radius), m_color(color), m_parentPlanet(parentPlanet), m_center(0.0f), m_mesh(nullptr) {
    // Generate the circle with 64 segments for visual smoothness
    m_mesh = std::make_unique<Mesh>(Renderer::createRing(m_radius, 64));
}

void Orbit::update(float deltaTime) {
    (void)deltaTime;
    if (m_parentPlanet) {
        m_center = m_parentPlanet->getPosition();
    } else {
        m_center = glm::vec3(0.0f);
    }
}

void Orbit::render(Renderer& renderer) {
    if (!m_mesh) return;

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, m_center);

    const Shader& shader = renderer.getShader();
    shader.use();
    shader.setBool("useColorOverride", true);
    shader.setVec3("colorOverride", m_color);

    // Orbit lines are drawn unlit
    renderer.render(*m_mesh, shader, model);

    shader.setBool("useColorOverride", false);
}
