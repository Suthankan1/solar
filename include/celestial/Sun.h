#pragma once

#include "scene/SceneObject.h"
#include "scene/Transform.h"
#include <glm/glm.hpp>

class Sun : public SceneObject {
public:
    Sun(const std::string& name, float radius, const glm::vec3& color, const glm::vec3& lightColor = glm::vec3(1.0f));
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
};
