#include "celestial/SaturnRings.h"
#include "celestial/Planet.h"
#include "core/Renderer.h"
#include "core/Shader.h"

SaturnRings::SaturnRings(const std::string& name, std::shared_ptr<Planet> parent)
    : SceneObject(name), m_parent(parent) {
    if (m_parent) {
        float parentRadius = m_parent->getRadius();
        m_innerRing = std::make_unique<Mesh>(Renderer::createRing(parentRadius * 1.4f, 128));
        m_middleRing = std::make_unique<Mesh>(Renderer::createRing(parentRadius * 1.8f, 128));
        m_outerRing = std::make_unique<Mesh>(Renderer::createRing(parentRadius * 2.3f, 128));
    }
}

void SaturnRings::update(float deltaTime) {
    (void)deltaTime;
    if (m_parent) {
        m_transform.setPosition(m_parent->getPosition());
        m_transform.setRotation(glm::vec3(glm::radians(27.0f), 0.0f, 0.0f));
    }
}

void SaturnRings::render(Renderer& renderer) {
    if (!m_innerRing || !m_middleRing || !m_outerRing) return;

    glm::mat4 model = m_transform.getModelMatrix();

    const Shader& shader = renderer.getShader();
    shader.use();
    shader.setBool("useColorOverride", true);

    // Inner ring rendering
    shader.setVec3("colorOverride", glm::vec3(0.85f, 0.78f, 0.62f));
    renderer.render(*m_innerRing, shader, model);

    // Middle ring rendering
    shader.setVec3("colorOverride", glm::vec3(0.78f, 0.70f, 0.55f));
    renderer.render(*m_middleRing, shader, model);

    // Outer ring rendering
    shader.setVec3("colorOverride", glm::vec3(0.65f, 0.58f, 0.44f));
    renderer.render(*m_outerRing, shader, model);

    shader.setBool("useColorOverride", false);
}
