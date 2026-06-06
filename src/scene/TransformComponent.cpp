#include "scene/TransformComponent.h"

TransformComponent::TransformComponent() 
    : m_transform(), m_parentComponent(nullptr) {
}

TransformComponent::TransformComponent(const glm::vec3& position, const glm::vec3& rotation, const glm::vec3& scale) 
    : m_transform(position, rotation, scale), m_parentComponent(nullptr) {
}

void TransformComponent::setParent(TransformComponent* parentComponent) {
    m_parentComponent = parentComponent;
    if (m_parentComponent) {
        m_transform.setParent(&m_parentComponent->getTransform());
    } else {
        m_transform.setParent(nullptr);
    }
}
