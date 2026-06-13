#include "celestial/AtmosphereLayer.h"
#include "celestial/Planet.h"
#include "core/Renderer.h"
#include "core/Shader.h"
#include <glad/glad.h>

AtmosphereLayer::AtmosphereLayer(const std::string& name, std::shared_ptr<Planet> earth, float radiusMultiplier, const glm::vec3& color)
    : SceneObject(name), m_earth(earth), m_radiusMultiplier(radiusMultiplier), m_color(color) {
    if (m_earth) {
        m_transform.setPosition(m_earth->getPosition());
        float radius = m_earth->getRadius() * m_radiusMultiplier;
        m_transform.setScale(glm::vec3(radius));
    }
}

void AtmosphereLayer::update(float deltaTime) {
    (void)deltaTime;
    if (m_earth) {
        m_transform.setPosition(m_earth->getPosition());
        float radius = m_earth->getRadius() * m_radiusMultiplier;
        m_transform.setScale(glm::vec3(radius));
    }
}

void AtmosphereLayer::render(Renderer& renderer) {
    if (!m_earth) return;

    glm::mat4 model = m_transform.getModelMatrix();

    // Enable transparent alpha blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Disable depth write to avoid sorting/clipping artifacts
    glDepthMask(GL_FALSE);

    if (renderer.getEarthShader() != nullptr) {
        const Shader& shader = *renderer.getEarthShader();
        shader.use();

        shader.setInt("earthMode", 2); // Mode 2: Atmosphere rendering

        renderer.renderWithLighting(renderer.getSphereMeshForRadius(m_transform.getScale().x, glm::distance(renderer.getCameraPosition(), m_transform.getPosition())), shader, model);
    } else {
        const Shader& shader = renderer.getShader();
        shader.use();

        shader.setInt("planetId", 14); // Atmosphere ID in shaders
        shader.setBool("useTexture", false);
        shader.setBool("useColorOverride", true);
        shader.setVec3("colorOverride", m_color);

        renderer.renderWithLighting(renderer.getSphereMeshForRadius(m_transform.getScale().x, glm::distance(renderer.getCameraPosition(), m_transform.getPosition())), shader, model);

        shader.setBool("useColorOverride", false);
    }

    // Restore standard OpenGL states
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}
