#include "celestial/SaturnRings.h"
#include "celestial/Planet.h"
#include "core/Renderer.h"
#include "core/Shader.h"
#include <glad/glad.h>
#include <glm/common.hpp>
#include <glm/geometric.hpp>

SaturnRings::SaturnRings(const std::string& name, std::shared_ptr<Planet> parent)
    : SceneObject(name), m_parent(parent) {
    if (m_parent) {
        float parentRadius = m_parent->getRadius();
        m_innerRing = std::make_unique<Mesh>(Renderer::createRing(parentRadius * kInnerRadiusMultiplier, 128));
        m_middleRing = std::make_unique<Mesh>(Renderer::createRing(parentRadius * kMiddleRadiusMultiplier, 128));
        m_outerRing = std::make_unique<Mesh>(Renderer::createRing(parentRadius * kOuterRadiusMultiplier, 128));
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
    glm::vec3 saturnPosition = m_parent ? m_parent->getPosition() : m_transform.getPosition();
    glm::vec3 sunDir = glm::normalize(renderer.getLightPosition() - saturnPosition);
    glm::vec3 viewDir = glm::normalize(saturnPosition - renderer.getCameraPosition());
    float backLight = glm::clamp(glm::dot(sunDir, viewDir), 0.0f, 1.0f);

    const Shader& shader = renderer.getShader();
    shader.use();
    shader.setBool("useColorOverride", true);

    const GLboolean blendWasEnabled = glIsEnabled(GL_BLEND);
    GLboolean depthWriteWasEnabled = GL_TRUE;
    glGetBooleanv(GL_DEPTH_WRITEMASK, &depthWriteWasEnabled);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);

    auto renderRingBand = [&](const Mesh& ring, const glm::vec3& color) {
        shader.setInt("planetId", 106);
        shader.setFloat("backLightFactor", backLight * 0.6f);
        shader.setFloat("globalAlpha", 0.72f);
        shader.setVec3("colorOverride", color);
        renderer.renderWithLighting(ring, shader, model);
    };

    // Inner ring rendering
    renderRingBand(*m_innerRing, glm::vec3(0.85f, 0.78f, 0.62f));

    // Middle ring rendering
    renderRingBand(*m_middleRing, glm::vec3(0.78f, 0.70f, 0.55f));

    // Outer ring rendering
    renderRingBand(*m_outerRing, glm::vec3(0.65f, 0.58f, 0.44f));

    shader.setBool("useColorOverride", false);
    shader.setFloat("backLightFactor", 0.0f);
    shader.setFloat("globalAlpha", 1.0f);

    glDepthMask(depthWriteWasEnabled);
    if (blendWasEnabled) {
        glEnable(GL_BLEND);
    } else {
        glDisable(GL_BLEND);
    }
}
