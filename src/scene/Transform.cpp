#define GLM_ENABLE_EXPERIMENTAL
#include "scene/Transform.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

Transform::Transform()
    : m_position(0.0f), m_rotation(0.0f), m_scale(1.0f), m_parent(nullptr), m_cachedModelMatrix(1.0f), m_dirty(true) {}

Transform::Transform(const glm::vec3& position, const glm::vec3& rotation, const glm::vec3& scale)
    : m_position(position), m_rotation(rotation), m_scale(scale), m_parent(nullptr), m_cachedModelMatrix(1.0f), m_dirty(true) {}

void Transform::setPosition(const glm::vec3& position) {
    m_position = position;
    m_dirty = true;
}

void Transform::setRotation(const glm::vec3& rotation) {
    m_rotation = rotation;
    m_dirty = true;
}

void Transform::setScale(const glm::vec3& scale) {
    m_scale = scale;
    m_dirty = true;
}

void Transform::translate(const glm::vec3& translation) {
    m_position += translation;
    m_dirty = true;
}

void Transform::rotate(const glm::vec3& rotationOffset) {
    m_rotation += rotationOffset;
    m_dirty = true;
}

void Transform::scale(const glm::vec3& scaleMultiplier) {
    m_scale *= scaleMultiplier;
    m_dirty = true;
}

void Transform::scale(float uniformScaleFactor) {
    m_scale *= uniformScaleFactor;
    m_dirty = true;
}

void Transform::setParent(Transform* parent) {
    m_parent = parent;
    m_dirty = true;
}

glm::mat4 Transform::getLocalModelMatrix() const {
    updateCachedMatrix();
    return m_cachedModelMatrix;
}

glm::mat4 Transform::getModelMatrix() const {
    if (m_parent) {
        return m_parent->getModelMatrix() * getLocalModelMatrix();
    }
    return getLocalModelMatrix();
}

void Transform::updateCachedMatrix() const {
    if (!m_dirty) {
        return;
    }

    // T * R * S
    glm::mat4 translationMat = glm::translate(glm::mat4(1.0f), m_position);
    
    // Convert Euler angles (Pitch/X, Yaw/Y, Roll/Z) to quaternion
    glm::quat q(m_rotation);
    glm::mat4 rotationMat = glm::toMat4(q);
    
    glm::mat4 scaleMat = glm::scale(glm::mat4(1.0f), m_scale);

    m_cachedModelMatrix = translationMat * rotationMat * scaleMat;
    m_dirty = false;
}

Transform Transform::lerp(const Transform& start, const Transform& end, float t) {
    Transform result;
    result.setPosition(glm::mix(start.m_position, end.m_position, t));
    result.setRotation(glm::mix(start.m_rotation, end.m_rotation, t));
    result.setScale(glm::mix(start.m_scale, end.m_scale, t));
    return result;
}

Transform Transform::interpolate(const Transform& start, const Transform& end, float t) {
    Transform result;
    result.setPosition(glm::mix(start.m_position, end.m_position, t));
    result.setScale(glm::mix(start.m_scale, end.m_scale, t));
    
    // Spherically interpolate rotations using quaternions
    glm::quat qStart(start.m_rotation);
    glm::quat qEnd(end.m_rotation);
    glm::quat qInterp = glm::slerp(qStart, qEnd, t);
    result.setRotation(glm::eulerAngles(qInterp));
    
    return result;
}
