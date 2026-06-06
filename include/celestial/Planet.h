#pragma once

#include "scene/SceneObject.h"
#include "scene/Transform.h"
#include <glm/glm.hpp>

class Planet : public SceneObject {
public:
    Planet(const std::string& name, float radius, float orbitRadius, float orbitSpeed, float rotationSpeed, const glm::vec3& color);
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

    float m_orbitAngle;
    float m_rotationAngle;
    Transform m_transform;
};
