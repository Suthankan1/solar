#include "celestial/Orbit.h"
#include "celestial/Planet.h"
#include "core/Renderer.h"
#include "core/Shader.h"
#include <glm/gtc/matrix_transform.hpp>

Orbit::Orbit(const std::string& name, float radius, const glm::vec3& color, std::shared_ptr<Planet> parentPlanet)
    : SceneObject(name), m_semiMajorAxis(radius), m_semiMinorAxis(radius), m_inclination(0.0f),
      m_longitudeOfAscendingNode(0.0f), m_color(color), m_parentPlanet(parentPlanet), m_center(0.0f), m_mesh(nullptr) {
    m_mesh = std::make_unique<Mesh>(Renderer::createEllipticalRing(m_semiMajorAxis, m_semiMinorAxis, m_inclination, m_longitudeOfAscendingNode, 256));
}

Orbit::Orbit(const std::string& name, float semiMajorAxis, float semiMinorAxis, float inclination, float longitudeOfAscendingNode, const glm::vec3& color, std::shared_ptr<Planet> parentPlanet)
    : SceneObject(name), m_semiMajorAxis(semiMajorAxis), m_semiMinorAxis(semiMinorAxis), m_inclination(inclination),
      m_longitudeOfAscendingNode(longitudeOfAscendingNode), m_color(color), m_parentPlanet(parentPlanet), m_center(0.0f), m_mesh(nullptr) {
    m_mesh = std::make_unique<Mesh>(Renderer::createEllipticalRing(m_semiMajorAxis, m_semiMinorAxis, m_inclination, m_longitudeOfAscendingNode, 256));
}

Orbit::Orbit(const std::string& name, std::shared_ptr<Planet> planetToTrack, const glm::vec3& color, std::shared_ptr<Planet> parentPlanet)
    : SceneObject(name), m_color(color), m_parentPlanet(parentPlanet), m_center(0.0f), m_mesh(nullptr) {
    if (planetToTrack) {
        m_semiMajorAxis = planetToTrack->getSemiMajorAxis();
        m_semiMinorAxis = planetToTrack->getSemiMinorAxis();
        m_inclination = planetToTrack->getInclination();
        m_longitudeOfAscendingNode = planetToTrack->getLongitudeOfAscendingNode();
    } else {
        m_semiMajorAxis = 1.0f;
        m_semiMinorAxis = 1.0f;
        m_inclination = 0.0f;
        m_longitudeOfAscendingNode = 0.0f;
    }
    m_mesh = std::make_unique<Mesh>(Renderer::createEllipticalRing(m_semiMajorAxis, m_semiMinorAxis, m_inclination, m_longitudeOfAscendingNode, 256));
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
    const Shader& shader = renderer.getShader();
    shader.use();

    auto resetShaderState = [&shader]() {
        shader.setBool("useColorOverride", false);
        shader.setFloat("globalAlpha", 1.0f);
        shader.setBool("useTexture", false);
        shader.setInt("planetId", -1);
    };

    if (!m_mesh) {
        resetShaderState();
        return;
    }

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, m_center);

    shader.setBool("useColorOverride", true);
    shader.setVec3("colorOverride", m_color);

    // Orbit lines are drawn unlit
    shader.setFloat("globalAlpha", 0.06f);
    glm::mat4 glowModel = glm::scale(model, glm::vec3(1.002f));
    renderer.render(*m_mesh, shader, glowModel);

    const float orbitAlpha = m_semiMajorAxis >= 7.0f ? 0.12f : 0.15f;
    shader.setFloat("globalAlpha", orbitAlpha);
    renderer.render(*m_mesh, shader, model);

    resetShaderState();
}
