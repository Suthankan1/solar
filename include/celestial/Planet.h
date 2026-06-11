#pragma once

#include "scene/SceneObject.h"
#include "scene/Transform.h"
#include "core/Texture.h"
#include <glm/glm.hpp>
#include <memory>

/**
 * @class Planet
 * @brief Represents a planet with orbital mechanics (revolution around a center and self-rotation).
 */
class Planet : public SceneObject {
public:
    /**
     * @brief Constructs a Planet with specified dimensions, orbital characteristics, and color.
     * @param name Planet name
     * @param radius Physical radius of the planet
     * @param orbitRadius Orbit radius from the center
     * @param orbitSpeed Orbital revolution speed
     * @param rotationSpeed Self-rotation speed
     * @param color Base rendering color
     * @param texturePath Path to texture image file
     */
    Planet(const std::string& name, float radius, float orbitRadius, float orbitSpeed, float rotationSpeed, const glm::vec3& color, const std::string& texturePath = "");
    
    /**
     * @brief Constructs a Planet with elliptical orbital characteristics.
     */
    Planet(const std::string& name, float radius, float semiMajorAxis, float semiMinorAxis, float inclination, float orbitSpeed, float rotationSpeed, const glm::vec3& color, const std::string& texturePath = "", float orbitPhaseOffset = 0.0f, float longitudeOfAscendingNode = 0.0f);
    
    virtual ~Planet() = default;

    void update(float deltaTime) override;
    void render(Renderer& renderer) override;

    const glm::vec3& getPosition() const { return m_transform.getPosition(); }
    float getRadius() const { return m_transform.getScale().x; }
    float getOrbitRadius() const { return m_semiMajorAxis; }

    float getSemiMajorAxis() const { return m_semiMajorAxis; }
    float getSemiMinorAxis() const { return m_semiMinorAxis; }
    float getInclination() const { return m_inclination; }
    float getOrbitPhaseOffset() const { return m_orbitPhaseOffset; }
    float getLongitudeOfAscendingNode() const { return m_longitudeOfAscendingNode; }
    float getOrbitAngle() const { return m_orbitAngle; }
    float getOrbitSpeed() const { return m_orbitSpeed; }

    Transform& getTransform() { return m_transform; }
    const Transform& getTransform() const { return m_transform; }

    const Texture* getTexture() const { return m_texture.get(); }
    const Texture* getNightTexture() const { return m_nightTexture.get(); }
    const Texture* getCloudTexture() const { return m_cloudTexture.get(); }
    const Texture* getSpecularTexture() const { return m_specularTexture.get(); }

private:
    float m_semiMajorAxis;
    float m_semiMinorAxis;
    float m_inclination;
    float m_orbitPhaseOffset;
    float m_longitudeOfAscendingNode;
    
    float m_orbitSpeed;
    float m_rotationSpeed;
    glm::vec3 m_color;
    std::unique_ptr<Texture> m_texture;

    std::unique_ptr<Texture> m_nightTexture;
    std::unique_ptr<Texture> m_cloudTexture;
    std::unique_ptr<Texture> m_specularTexture;

    float m_orbitAngle;
    float m_rotationAngle;
    Transform m_transform;
};
