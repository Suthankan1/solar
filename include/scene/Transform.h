#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

class Transform {
public:
    // Constructors
    Transform();
    Transform(const glm::vec3& position, const glm::vec3& rotation = glm::vec3(0.0f), const glm::vec3& scale = glm::vec3(1.0f));

    // Getters and Setters
    const glm::vec3& getPosition() const { return m_position; }
    void setPosition(const glm::vec3& position);

    const glm::vec3& getRotation() const { return m_rotation; } // Euler angles in radians
    void setRotation(const glm::vec3& rotation);

    const glm::vec3& getScale() const { return m_scale; }
    void setScale(const glm::vec3& scale);

    // Transformation operations
    void translate(const glm::vec3& translation);
    void rotate(const glm::vec3& rotationOffset);
    void scale(const glm::vec3& scaleMultiplier);
    void scale(float uniformScaleFactor);

    // Matrix calculations
    glm::mat4 getLocalModelMatrix() const;
    glm::mat4 getModelMatrix() const;

    // Hierarchy management
    void setParent(Transform* parent);
    Transform* getParent() const { return m_parent; }

    // Animation / Interpolation helpers
    static Transform lerp(const Transform& start, const Transform& end, float t);
    static Transform interpolate(const Transform& start, const Transform& end, float t);

private:
    glm::vec3 m_position;
    glm::vec3 m_rotation; // Euler angles in radians (Pitch/X, Yaw/Y, Roll/Z)
    glm::vec3 m_scale;

    Transform* m_parent;

    // Caching system for performance optimization
    mutable glm::mat4 m_cachedModelMatrix;
    mutable bool m_dirty;

    void updateCachedMatrix() const;
};
