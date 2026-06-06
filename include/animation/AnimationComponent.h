#pragma once

#include "scene/Component.h"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <vector>
#include <string>

// Structs for future keyframe animation track support
struct VectorKeyframe {
    float time;
    glm::vec3 value;
};

struct QuatKeyframe {
    float time;
    glm::quat value;
};

class AnimationComponent : public Component {
public:
    AnimationComponent();
    virtual ~AnimationComponent() = default;

    // Overridden from Component
    void update(float deltaTime) override;

    // Procedural spin configurations
    void setSelfRotationSpeed(float speed) { m_selfRotationSpeed = speed; }
    float getSelfRotationSpeed() const { return m_selfRotationSpeed; }

    // Procedural orbit configurations
    void setOrbit(Entity* centerEntity, float radius, float speed);
    void setOrbitStatic(const glm::vec3& centerPos, float radius, float speed);

    // Track Animation interface (design for future expansion)
    void addPositionKeyframe(float time, const glm::vec3& position);
    void addRotationKeyframe(float time, const glm::quat& rotation);
    void playTrack(const std::string& name);
    void pause();
    void stop();

private:
    // Procedural animation state
    float m_selfRotationSpeed; // in radians/sec
    
    Entity* m_orbitCenterEntity;
    glm::vec3 m_orbitCenterStatic;
    float m_orbitRadius;
    float m_orbitSpeed;
    float m_orbitAngle;

    // Keyframe tracks and playback state
    std::vector<VectorKeyframe> m_positionTrack;
    std::vector<QuatKeyframe> m_rotationTrack;
    bool m_isPlaying;
    float m_trackTime;
    std::string m_activeTrackName;

    // Helper functions for keyframe interpolation
    glm::vec3 interpolatePosition(float time);
    glm::quat interpolateRotation(float time);
};
