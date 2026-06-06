#pragma once

#include "scene/SceneObject.h"
#include <glm/glm.hpp>
#include <memory>

class Camera : public SceneObject {
public:
    Camera(const std::string& name, const glm::vec3& position, const glm::vec3& target, const glm::vec3& up = glm::vec3(0.0f, 1.0f, 0.0f));
    virtual ~Camera() = default;

    virtual glm::mat4 getViewMatrix() const;
    virtual glm::mat4 getProjectionMatrix(float aspect) const;

    const glm::vec3& getPosition() const { return m_position; }
    void setPosition(const glm::vec3& pos) { m_position = pos; }

    const glm::vec3& getFront() const { return m_front; }
    void setFront(const glm::vec3& front) { m_front = front; }

    const glm::vec3& getUp() const { return m_up; }
    void setUp(const glm::vec3& up) { m_up = up; }

    float getFOV() const { return m_fov; }
    void setFOV(float fov) { m_fov = fov; }

    float getNearPlane() const { return m_nearPlane; }
    float getFarPlane() const { return m_farPlane; }

protected:
    glm::vec3 m_position;
    glm::vec3 m_front;
    glm::vec3 m_up;
    float m_fov;         // in degrees
    float m_nearPlane;
    float m_farPlane;
};

// 1. StaticCamera: Fixed camera looking at a specific point
class StaticCamera : public Camera {
public:
    StaticCamera(const std::string& name, const glm::vec3& position, const glm::vec3& target = glm::vec3(0.0f));
    void update(float) override {}
    void render(Renderer&) override {}
};

// 2. OrbitingCamera: Auto-orbits around the origin
class OrbitingCamera : public Camera {
public:
    OrbitingCamera(const std::string& name, float distance, float speed, float height = 2.0f);
    void update(float deltaTime) override;
    void render(Renderer&) override {}
private:
    float m_distance;
    float m_speed;
    float m_height;
    float m_angle;
};

// 3. TrackingCamera: Tracks a moving target planet
class Planet;
class TrackingCamera : public Camera {
public:
    TrackingCamera(const std::string& name, std::shared_ptr<Planet> target, const glm::vec3& offset);
    void update(float deltaTime) override;
    void render(Renderer&) override {}
private:
    std::shared_ptr<Planet> m_targetPlanet;
    glm::vec3 m_offset;
};

// 4. FreeCamera: Fly-through camera controlled by W/S/A/D, Q/E, and Arrow keys
class FreeCamera : public Camera {
public:
    FreeCamera(const std::string& name, const glm::vec3& position);
    void update(float deltaTime) override;
    void render(Renderer&) override {}
private:
    float m_yaw;
    float m_pitch;
    float m_movementSpeed;
};
