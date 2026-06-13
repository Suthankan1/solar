#include "celestial/Planet.h"
#include "celestial/OrbitMath.h"
#include "core/Renderer.h"
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>
#include <cstdlib>

Planet::Planet(const std::string& name, float radius, float orbitRadius, float orbitSpeed, float rotationSpeed, const glm::vec3& color, const std::string& texturePath)
    : Planet(name, radius, orbitRadius, orbitRadius, 0.0f, orbitSpeed, rotationSpeed, color, texturePath,
             static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX) * 2.0f * 3.1415926f, 0.0f) {
}

Planet::Planet(const std::string& name, float radius, float semiMajorAxis, float semiMinorAxis, float inclination, float orbitSpeed, float rotationSpeed, const glm::vec3& color, const std::string& texturePath, float orbitPhaseOffset, float longitudeOfAscendingNode)
    : SceneObject(name), m_semiMajorAxis(semiMajorAxis), m_semiMinorAxis(semiMinorAxis), m_inclination(inclination),
      m_orbitPhaseOffset(orbitPhaseOffset), m_longitudeOfAscendingNode(longitudeOfAscendingNode),
      m_orbitSpeed(orbitSpeed), m_rotationSpeed(rotationSpeed), m_color(color), m_orbitAngle(orbitPhaseOffset), m_rotationAngle(0.0f) {
    m_transform.setScale(glm::vec3(radius));

    if (!texturePath.empty()) {
        m_texture = std::make_unique<Texture>();
        if (!m_texture->load(texturePath)) {
            m_texture.reset(); // Falls back to color if load fails
        }
    }

    if (m_name == "Earth") {
        m_nightTexture = std::make_unique<Texture>();
        if (!m_nightTexture->load("textures/earth_night.jpg")) {
            m_nightTexture.reset();
        }
        m_cloudTexture = std::make_unique<Texture>();
        if (!m_cloudTexture->load("textures/earth_clouds.jpg")) {
            m_cloudTexture.reset();
        }
        m_specularTexture = std::make_unique<Texture>();
        if (!m_specularTexture->load("textures/earth_specular.jpg")) {
            m_specularTexture.reset();
        }
    }
}

void Planet::update(float deltaTime) {
    m_orbitAngle += m_orbitSpeed * deltaTime;
    m_rotationAngle += m_rotationSpeed * deltaTime;

    m_transform.setPosition(calculateOrbitPosition(
        m_semiMajorAxis,
        m_semiMinorAxis,
        m_inclination,
        m_longitudeOfAscendingNode,
        m_orbitAngle
    ));
    m_transform.setRotation(glm::vec3(0.0f, m_rotationAngle, 0.0f));
}

void Planet::render(Renderer& renderer) {
    const float cameraDistance = glm::distance(renderer.getCameraPosition(), getPosition());
    if (!renderer.sphereInFrustum(getPosition(), getRadius() * 1.75f)) {
        return;
    }

    float tiltDeg = 0.0f;
    if (m_name == "Earth") tiltDeg = 23.4f;
    else if (m_name == "Mars") tiltDeg = 25.2f;
    else if (m_name == "Jupiter") tiltDeg = 3.1f;
    else if (m_name == "Saturn") tiltDeg = 26.7f;
    else if (m_name == "Uranus") tiltDeg = 97.8f;
    else if (m_name == "Neptune") tiltDeg = 28.3f;
    else if (m_name == "Venus") tiltDeg = 177.4f;
    else if (m_name == "Mercury") tiltDeg = 0.034f;

    glm::mat4 model = m_transform.getModelMatrix();
    model = glm::rotate(model, glm::radians(tiltDeg), glm::vec3(0.0f, 0.0f, 1.0f));

    if (m_name == "Earth" && renderer.getEarthShader() != nullptr) {
        const Shader& shader = *renderer.getEarthShader();
        shader.use();

        shader.setInt("earthMode", 0); // Mode 0: Surface rendering

        bool hasTexture = m_texture && m_texture->isValid();
        shader.setBool("useTexture", hasTexture);
        if (hasTexture) {
            m_texture->bind(0);
            shader.setInt("planetTexture", 0);
        }

        bool hasNight = m_nightTexture && m_nightTexture->isValid();
        shader.setBool("useNightTexture", hasNight);
        if (hasNight) {
            m_nightTexture->bind(1);
            shader.setInt("nightTexture", 1);
        }

        bool hasCloud = m_cloudTexture && m_cloudTexture->isValid();
        shader.setBool("useCloudTexture", hasCloud);
        if (hasCloud) {
            m_cloudTexture->bind(2);
            shader.setInt("cloudTexture", 2);
        }

        bool hasSpec = m_specularTexture && m_specularTexture->isValid();
        shader.setBool("useSpecularTexture", hasSpec);
        if (hasSpec) {
            m_specularTexture->bind(3);
            shader.setInt("specularTexture", 3);
        }

        // Earth is lit by the central star
        renderer.renderWithLighting(renderer.getSphereMeshForRadius(getRadius(), cameraDistance), shader, model);

        // Unbind to prevent state bleed
        if (hasTexture) m_texture->unbind();
        if (hasNight) m_nightTexture->unbind();
        if (hasCloud) m_cloudTexture->unbind();
        if (hasSpec) m_specularTexture->unbind();
    } else {
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
        renderer.renderWithLighting(renderer.getSphereMeshForRadius(getRadius(), cameraDistance), shader, model);

        shader.setBool("useColorOverride", false);
        if (hasTexture) {
            shader.setBool("useTexture", false);
        }
    }
}
