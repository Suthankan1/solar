#include "celestial/CloudLayer.h"
#include "celestial/Planet.h"
#include "core/Renderer.h"
#include "core/Shader.h"
#include <glad/glad.h>

CloudLayer::CloudLayer(const std::string& name, std::shared_ptr<Planet> earth, float radiusMultiplier, float rotationSpeed)
    : SceneObject(name), m_earth(earth), m_radiusMultiplier(radiusMultiplier),
      m_rotationSpeed(rotationSpeed), m_rotationAngle(0.0f) {
    if (m_earth) {
        m_transform.setPosition(m_earth->getPosition());
        float radius = m_earth->getRadius() * m_radiusMultiplier;
        m_transform.setScale(glm::vec3(radius));
    }
}

void CloudLayer::update(float deltaTime) {
    if (m_earth) {
        m_transform.setPosition(m_earth->getPosition());
        float radius = m_earth->getRadius() * m_radiusMultiplier;
        m_transform.setScale(glm::vec3(radius));
    }
    m_rotationAngle += m_rotationSpeed * deltaTime;
    m_transform.setRotation(glm::vec3(0.0f, m_rotationAngle, 0.0f));
}

void CloudLayer::render(Renderer& renderer) {
    if (!m_earth) return;

    glm::mat4 model = m_transform.getModelMatrix();

    // Enable transparent alpha blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Disable depth write so clouds do not block other transparent layers or geometry
    glDepthMask(GL_FALSE);

    if (renderer.getEarthShader() != nullptr) {
        const Shader& shader = *renderer.getEarthShader();
        shader.use();

        shader.setInt("earthMode", 1); // Mode 1: Cloud rendering

        const Texture* cloudTex = m_earth->getCloudTexture();
        bool hasTexture = cloudTex && cloudTex->isValid();
        shader.setBool("useCloudTexture", hasTexture);
        if (hasTexture) {
            cloudTex->bind(2);
            shader.setInt("cloudTexture", 2);
        }

        renderer.renderWithLighting(renderer.getSphereMesh(), shader, model);

        if (hasTexture) {
            cloudTex->unbind();
        }
    } else {
        const Shader& shader = renderer.getShader();
        shader.use();

        shader.setInt("planetId", 13); // CloudLayer ID in standard shaders
        shader.setBool("useTexture", false);
        shader.setBool("useColorOverride", false);

        renderer.renderWithLighting(renderer.getSphereMesh(), shader, model);
    }

    // Restore standard OpenGL states
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}
