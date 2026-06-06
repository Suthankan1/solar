#include "animation/AnimationComponent.h"
#include "scene/Entity.h"
#include "scene/TransformComponent.h"
#include <cmath>

AnimationComponent::AnimationComponent()
    : m_selfRotationSpeed(0.0f),
      m_orbitCenterEntity(nullptr),
      m_orbitCenterStatic(0.0f),
      m_orbitRadius(0.0f),
      m_orbitSpeed(0.0f),
      m_orbitAngle(0.0f),
      m_isPlaying(false),
      m_trackTime(0.0f) {
}

void AnimationComponent::setOrbit(Entity* centerEntity, float radius, float speed) {
    m_orbitCenterEntity = centerEntity;
    m_orbitRadius = radius;
    m_orbitSpeed = speed;
}

void AnimationComponent::setOrbitStatic(const glm::vec3& centerPos, float radius, float speed) {
    m_orbitCenterEntity = nullptr;
    m_orbitCenterStatic = centerPos;
    m_orbitRadius = radius;
    m_orbitSpeed = speed;
}

void AnimationComponent::addPositionKeyframe(float time, const glm::vec3& position) {
    m_positionTrack.push_back({time, position});
}

void AnimationComponent::addRotationKeyframe(float time, const glm::quat& rotation) {
    m_rotationTrack.push_back({time, rotation});
}

void AnimationComponent::playTrack(const std::string& name) {
    m_activeTrackName = name;
    m_isPlaying = true;
    m_trackTime = 0.0f;
}

void AnimationComponent::pause() {
    m_isPlaying = false;
}

void AnimationComponent::stop() {
    m_isPlaying = false;
    m_trackTime = 0.0f;
}

void AnimationComponent::update(float deltaTime) {
    TransformComponent* tc = m_owner->getComponent<TransformComponent>();
    if (!tc) return;

    // 1. Process Procedural Spin
    if (m_selfRotationSpeed != 0.0f) {
        tc->rotate(glm::vec3(0.0f, m_selfRotationSpeed * deltaTime, 0.0f));
    }

    // 2. Process Procedural Orbit
    if (m_orbitRadius > 0.0f) {
        m_orbitAngle += m_orbitSpeed * deltaTime;
        
        glm::vec3 center = m_orbitCenterStatic;
        if (m_orbitCenterEntity) {
            TransformComponent* centerTC = m_orbitCenterEntity->getComponent<TransformComponent>();
            if (centerTC) {
                center = centerTC->getPosition();
            }
        }

        float x = center.x + m_orbitRadius * std::cos(m_orbitAngle);
        float z = center.z + m_orbitRadius * std::sin(m_orbitAngle);
        tc->setPosition(glm::vec3(x, tc->getPosition().y, z));
    }

    // 3. Process Track Animation playback (if active)
    if (m_isPlaying) {
        m_trackTime += deltaTime;
        
        if (!m_positionTrack.empty()) {
            tc->setPosition(interpolatePosition(m_trackTime));
        }
        if (!m_rotationTrack.empty()) {
            // Convert quaternion interpolation to euler angles (or quaternion updates directly in future)
            glm::quat q = interpolateRotation(m_trackTime);
            tc->setRotation(glm::eulerAngles(q));
        }
    }
}

glm::vec3 AnimationComponent::interpolatePosition(float time) {
    if (m_positionTrack.empty()) return glm::vec3(0.0f);
    if (m_positionTrack.size() == 1 || time <= m_positionTrack.front().time) {
        return m_positionTrack.front().value;
    }
    if (time >= m_positionTrack.back().time) {
        return m_positionTrack.back().value;
    }

    // Find the current keyframe segment
    for (size_t i = 0; i < m_positionTrack.size() - 1; ++i) {
        if (time >= m_positionTrack[i].time && time <= m_positionTrack[i+1].time) {
            float t = (time - m_positionTrack[i].time) / (m_positionTrack[i+1].time - m_positionTrack[i].time);
            return glm::mix(m_positionTrack[i].value, m_positionTrack[i+1].value, t);
        }
    }
    return m_positionTrack.back().value;
}

glm::quat AnimationComponent::interpolateRotation(float time) {
    if (m_rotationTrack.empty()) return glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    if (m_rotationTrack.size() == 1 || time <= m_rotationTrack.front().time) {
        return m_rotationTrack.front().value;
    }
    if (time >= m_rotationTrack.back().time) {
        return m_rotationTrack.back().value;
    }

    // Find the current keyframe segment
    for (size_t i = 0; i < m_rotationTrack.size() - 1; ++i) {
        if (time >= m_rotationTrack[i].time && time <= m_rotationTrack[i+1].time) {
            float t = (time - m_rotationTrack[i].time) / (m_rotationTrack[i+1].time - m_rotationTrack[i].time);
            return glm::slerp(m_rotationTrack[i].value, m_rotationTrack[i+1].value, t);
        }
    }
    return m_rotationTrack.back().value;
}
