#pragma once

#include "scene/SceneObject.h"
#include "scene/Transform.h"
#include "core/Texture.h"
#include <glm/glm.hpp>
#include <memory>

class Planet;

/**
 * @class Moon
 * @brief Represents a natural satellite that transforms relative to its parent planet using the Transform hierarchy.
 */
class Moon : public SceneObject {
public:
    Moon(const std::string& name, float radius, float orbitRadius, float orbitSpeed, float rotationSpeed, const glm::vec3& color, std::shared_ptr<Planet> parentPlanet, const std::string& texturePath = "");
    virtual ~Moon() = default;

    void update(float deltaTime) override;
    void render(Renderer& renderer) override;

    glm::vec3 getPosition() const { return glm::vec3(m_transform.getModelMatrix()[3]); }
    float getRadius() const { return m_transform.getScale().x; }
    float getOrbitRadius() const { return m_orbitRadius; }

    Transform& getTransform() { return m_transform; }
    const Transform& getTransform() const { return m_transform; }

private:
    float m_orbitRadius;
    float m_orbitSpeed;
    float m_rotationSpeed;
    glm::vec3 m_color;
    std::shared_ptr<Planet> m_parentPlanet;

    float m_orbitAngle;
    float m_rotationAngle;
    Transform m_transform;
    std::unique_ptr<Texture> m_texture;
};
