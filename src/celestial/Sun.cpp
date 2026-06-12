#include "celestial/Sun.h"
#include "core/Renderer.h"
#include "core/Shader.h"
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>

Sun::Sun(const std::string& name, float radius, const glm::vec3& color, const glm::vec3& lightColor, const std::string& texturePath)
    : SceneObject(name), m_color(color), m_lightColor(lightColor) {
    m_transform.setPosition(glm::vec3(0.0f));
    m_transform.setScale(glm::vec3(radius));

    if (!texturePath.empty()) {
        m_texture = std::make_unique<Texture>();
        if (!m_texture->load(texturePath)) {
            m_texture.reset(); // Falls back to color if load fails
        }
    }
}

void Sun::update(float deltaTime) {
    m_rotationAngle += m_rotationSpeed * deltaTime;
}

void Sun::render(Renderer& renderer) {
    // Register the star's position and color as the main light source for planets/moons
    renderer.setLightSource(m_transform.getPosition(), m_lightColor);

    m_transform.setRotation(glm::vec3(0.0f, m_rotationAngle, 0.0f));
    glm::mat4 model = m_transform.getModelMatrix();

    const Shader& shader = renderer.getShader();
    shader.use();

    bool hasTexture = m_texture && m_texture->isValid();
    shader.setBool("useTexture", hasTexture);
    if (hasTexture) {
        m_texture->bind(0);
        shader.setInt("planetTexture", 0);
    }

    shader.setFloat("emissiveStrength", 0.6f);
    shader.setBool("useColorOverride", true);
    shader.setVec3("colorOverride", m_color);
    shader.setInt("planetId", 0);
    shader.setFloat("time", static_cast<float>(glfwGetTime()));

    // Star does not get lit by itself, render it unlit
    renderer.render(renderer.getSphereMesh(), shader, model);

    // Reset override
    if (hasTexture) {
        shader.setBool("useTexture", false);
    }
    shader.setBool("useColorOverride", false);
    shader.setFloat("emissiveStrength", 0.0f);
}
