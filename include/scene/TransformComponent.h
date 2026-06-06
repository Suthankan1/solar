#pragma once

#include "scene/Component.h"
#include "scene/Transform.h"
#include <glm/glm.hpp>

class TransformComponent : public Component {
public:
    TransformComponent();
    TransformComponent(const glm::vec3& position, 
                       const glm::vec3& rotation = glm::vec3(0.0f), 
                       const glm::vec3& scale = glm::vec3(1.0f));
    virtual ~TransformComponent() = default;

    // Delegate Transform methods
    const glm::vec3& getPosition() const { return m_transform.getPosition(); }
    void setPosition(const glm::vec3& position) { m_transform.setPosition(position); }

    const glm::vec3& getRotation() const { return m_transform.getRotation(); }
    void setRotation(const glm::vec3& rotation) { m_transform.setRotation(rotation); }

    const glm::vec3& getScale() const { return m_transform.getScale(); }
    void setScale(const glm::vec3& scale) { m_transform.setScale(scale); }

    void translate(const glm::vec3& translation) { m_transform.translate(translation); }
    void rotate(const glm::vec3& rotationOffset) { m_transform.rotate(rotationOffset); }
    void scale(const glm::vec3& scaleMultiplier) { m_transform.scale(scaleMultiplier); }
    void scale(float uniformScaleFactor) { m_transform.scale(uniformScaleFactor); }

    glm::mat4 getModelMatrix() const { return m_transform.getModelMatrix(); }
    glm::mat4 getLocalModelMatrix() const { return m_transform.getLocalModelMatrix(); }

    void setParent(TransformComponent* parentComponent);
    TransformComponent* getParentComponent() const { return m_parentComponent; }

    Transform& getTransform() { return m_transform; }
    const Transform& getTransform() const { return m_transform; }

private:
    Transform m_transform;
    TransformComponent* m_parentComponent = nullptr;
};
