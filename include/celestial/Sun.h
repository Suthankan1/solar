#pragma once

#include "scene/SceneObject.h"
#include "scene/Transform.h"
#include "core/Texture.h"
#include <glm/glm.hpp>
#include <memory>

/**
 * @class Sun
 * @brief Represents the central star that acts as the scene's primary light source.
 */
class Sun : public SceneObject {
public:
    Sun(const std::string& name, float radius, const glm::vec3& color, const glm::vec3& lightColor = glm::vec3(1.0f), const std::string& texturePath = "");
    virtual ~Sun() = default;

    void update(float deltaTime) override;
    void render(Renderer& renderer) override;

    const glm::vec3& getPosition() const { return m_transform.getPosition(); }
    const glm::vec3& getLightColor() const { return m_lightColor; }

    Transform& getTransform() { return m_transform; }
    const Transform& getTransform() const { return m_transform; }

private:
    glm::vec3 m_color;
    glm::vec3 m_lightColor;
    Transform m_transform;
    float m_rotationAngle = 0.0f;
    float m_rotationSpeed = 0.1f;
    std::unique_ptr<Texture> m_texture;
};
