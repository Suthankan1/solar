#include "celestial/Orbit.h"
#include "celestial/OrbitMath.h"
#include "celestial/Planet.h"
#include "core/Renderer.h"
#include "core/Shader.h"
#include <glad/glad.h>
#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>

Orbit::Orbit(const std::string& name, float radius, const glm::vec3& color, std::shared_ptr<Planet> parentPlanet)
    : SceneObject(name), m_semiMajorAxis(radius), m_semiMinorAxis(radius), m_inclination(0.0f),
      m_longitudeOfAscendingNode(0.0f), m_color(color), m_parentPlanet(parentPlanet), m_center(0.0f),
      m_isLocalOrbit(parentPlanet != nullptr), m_emphasized(false), m_highQuality(true), m_planeOffset(0.0f), m_alphaMultiplier(1.0f), m_mesh(nullptr) {
    rebuildMesh(kDefaultOrbitSegments);
}

Orbit::Orbit(const std::string& name, float semiMajorAxis, float semiMinorAxis, float inclination, float longitudeOfAscendingNode, const glm::vec3& color, std::shared_ptr<Planet> parentPlanet)
    : SceneObject(name), m_semiMajorAxis(semiMajorAxis), m_semiMinorAxis(semiMinorAxis), m_inclination(inclination),
      m_longitudeOfAscendingNode(longitudeOfAscendingNode), m_color(color), m_parentPlanet(parentPlanet), m_center(0.0f),
      m_isLocalOrbit(parentPlanet != nullptr), m_emphasized(false), m_highQuality(true), m_planeOffset(0.0f), m_alphaMultiplier(1.0f), m_mesh(nullptr) {
    rebuildMesh(kDefaultOrbitSegments);
}

Orbit::Orbit(const std::string& name, std::shared_ptr<Planet> planetToTrack, const glm::vec3& color, std::shared_ptr<Planet> parentPlanet)
    : SceneObject(name), m_color(color), m_parentPlanet(parentPlanet), m_center(0.0f),
      m_isLocalOrbit(parentPlanet != nullptr), m_emphasized(false), m_highQuality(true), m_planeOffset(0.0f), m_alphaMultiplier(1.0f), m_mesh(nullptr) {
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
    rebuildMesh(kDefaultOrbitSegments);
}

void Orbit::update(float deltaTime) {
    (void)deltaTime;
    if (m_parentPlanet) {
        m_center = m_parentPlanet->getPosition();
    } else {
        m_center = glm::vec3(0.0f);
    }
}

void Orbit::setLocalOrbitStyle(float planeOffset, const glm::vec3& color, float alphaMultiplier) {
    m_isLocalOrbit = true;
    m_planeOffset = planeOffset;
    m_color = color;
    m_alphaMultiplier = std::max(alphaMultiplier, 0.1f);
}

void Orbit::setHighQuality(bool enabled) {
    if (m_highQuality == enabled) {
        return;
    }
    m_highQuality = enabled;
    rebuildMesh(enabled ? kDefaultOrbitSegments : 180u);
}

void Orbit::rebuildMesh(unsigned int segments) {
    m_mesh = std::make_unique<Mesh>(Renderer::createEllipticalRing(
        m_semiMajorAxis,
        m_semiMinorAxis,
        m_inclination,
        m_longitudeOfAscendingNode,
        segments
    ));
}

float Orbit::calculateAdaptiveAlpha(const Renderer& renderer) const {
    const float cameraDistance = glm::distance(renderer.getCameraPosition(), m_center);
    const float orbitRadius = std::max(m_semiMajorAxis, m_semiMinorAxis);
    const float edgeDistance = std::max(cameraDistance - orbitRadius, 0.0f);
    const float fadeRange = std::max(orbitRadius * 1.5f, 1.0f);
    const float nearFactor = 1.0f - std::clamp(edgeDistance / fadeRange, 0.0f, 1.0f);

    const float farAlpha = m_isLocalOrbit ? 0.18f : 0.085f;
    const float nearAlpha = m_emphasized ? 0.36f : (m_isLocalOrbit ? 0.34f : 0.24f);
    const float emphasisBoost = m_emphasized ? 1.18f : 1.0f;
    return std::clamp(glm::mix(farAlpha, nearAlpha, nearFactor) * m_alphaMultiplier * emphasisBoost, farAlpha, 0.42f);
}

void Orbit::render(Renderer& renderer) {
    const Shader& shader = renderer.getShader();
    shader.use();

    auto resetShaderState = [&shader]() {
        shader.setBool("useColorOverride", false);
        shader.setVec3("colorOverride", glm::vec3(1.0f));
        shader.setFloat("globalAlpha", 1.0f);
        shader.setBool("useTexture", false);
        shader.setBool("useLighting", true);
        shader.setInt("planetId", -1);
        shader.setFloat("emissiveStrength", 0.0f);
        shader.setFloat("backLightFactor", 0.0f);
        shader.setBool("isAsteroidPointSprite", false);
    };

    if (!m_mesh) {
        resetShaderState();
        return;
    }

    const GLboolean blendWasEnabled = glIsEnabled(GL_BLEND);
    const GLboolean depthWasEnabled = glIsEnabled(GL_DEPTH_TEST);
    GLboolean depthWriteWasEnabled = GL_TRUE;
    GLint blendSrcRgb = GL_SRC_ALPHA;
    GLint blendDstRgb = GL_ONE_MINUS_SRC_ALPHA;
    GLint blendSrcAlpha = GL_SRC_ALPHA;
    GLint blendDstAlpha = GL_ONE_MINUS_SRC_ALPHA;
    glGetBooleanv(GL_DEPTH_WRITEMASK, &depthWriteWasEnabled);
    glGetIntegerv(GL_BLEND_SRC_RGB, &blendSrcRgb);
    glGetIntegerv(GL_BLEND_DST_RGB, &blendDstRgb);
    glGetIntegerv(GL_BLEND_SRC_ALPHA, &blendSrcAlpha);
    glGetIntegerv(GL_BLEND_DST_ALPHA, &blendDstAlpha);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);

    glm::mat4 model = glm::mat4(1.0f);
    // Solar orbit rings are centered on the Sun. Local rings are centered on
    // their parent planet's current world position; the mesh itself stays in
    // local orbit space to avoid accidental double transforms.
    const glm::vec3 planeOffset = calculateOrbitPlaneNormal(m_inclination, m_longitudeOfAscendingNode) * m_planeOffset;
    model = glm::translate(model, m_center + planeOffset);

    shader.setBool("useColorOverride", true);
    shader.setVec3("colorOverride", m_color);

    const float orbitAlpha = calculateAdaptiveAlpha(renderer);

    // Orbit tubes are drawn unlit. A faint outer pass keeps edge-on views from
    // flickering away while the main pass remains restrained.
    shader.setFloat("globalAlpha", std::max(0.045f, orbitAlpha * 0.35f));
    glm::mat4 glowModel = glm::scale(model, glm::vec3(1.002f));
    renderer.render(*m_mesh, shader, glowModel);

    shader.setFloat("globalAlpha", orbitAlpha);
    renderer.render(*m_mesh, shader, model);

    resetShaderState();

    if (blendWasEnabled) {
        glEnable(GL_BLEND);
    } else {
        glDisable(GL_BLEND);
    }
    if (depthWasEnabled) {
        glEnable(GL_DEPTH_TEST);
    } else {
        glDisable(GL_DEPTH_TEST);
    }
    glDepthMask(depthWriteWasEnabled);
    glBlendFuncSeparate(blendSrcRgb, blendDstRgb, blendSrcAlpha, blendDstAlpha);
}
