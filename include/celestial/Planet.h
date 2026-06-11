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
    virtual ~Planet() = default;

    void update(float deltaTime) override;
    void render(Renderer& renderer) override;

    const glm::vec3& getPosition() const { return m_transform.getPosition(); }
    float getRadius() const { return m_transform.getScale().x; }
    float getOrbitRadius() const { return m_orbitRadius; }

    Transform& getTransform() { return m_transform; }
    const Transform& getTransform() const { return m_transform; }

private:
    float m_orbitRadius;
    float m_orbitSpeed;
    float m_rotationSpeed;
    glm::vec3 m_color;
    std::unique_ptr<Texture> m_texture;

    float m_orbitAngle;
    float m_rotationAngle;
    Transform m_transform;
};
