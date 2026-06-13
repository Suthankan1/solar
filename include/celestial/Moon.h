#pragma once

#include "scene/SceneObject.h"
#include "scene/Transform.h"
#include "core/Texture.h"
#include <glm/glm.hpp>
#include <memory>

class Planet;

/**
 * @class Moon
 * @brief Represents a natural satellite orbiting a parent planet in world space.
 */
class Moon : public SceneObject {
public:
    Moon(const std::string& name,
         float radius,
         float orbitRadius,
         float orbitSpeed,
         float rotationSpeed,
         const glm::vec3& color,
         std::shared_ptr<Planet> parentPlanet,
         const std::string& texturePath = "",
         float orbitPhaseOffset = 0.0f,
         float orbitInclination = 0.0f,
         float longitudeOfAscendingNode = 0.0f);
    virtual ~Moon() = default;

    void update(float deltaTime) override;
    void render(Renderer& renderer) override;

    glm::vec3 getPosition() const { return m_worldPosition; }
    float getRadius() const { return m_transform.getScale().x; }
    float getOrbitRadius() const { return m_orbitRadius; }
    float getOrbitInclination() const { return m_orbitInclination; }
    float getLongitudeOfAscendingNode() const { return m_longitudeOfAscendingNode; }

    Transform& getTransform() { return m_transform; }
    const Transform& getTransform() const { return m_transform; }

private:
    float m_orbitRadius;
    float m_orbitSpeed;
    float m_rotationSpeed;
    float m_orbitInclination;
    float m_longitudeOfAscendingNode;
    glm::vec3 m_color;
    std::shared_ptr<Planet> m_parentPlanet;

    float m_orbitAngle;
    float m_rotationAngle;
    glm::vec3 m_worldPosition;
    Transform m_transform;
    std::unique_ptr<Texture> m_texture;
    bool m_warnedAboutParentOverlap;
};
