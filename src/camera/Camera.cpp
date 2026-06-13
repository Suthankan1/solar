#include "camera/Camera.h"
#include "celestial/Planet.h"
#include "celestial/Spacecraft.h"
#include "core/Window.h"
#include <glm/gtc/matrix_transform.hpp>
#include <GLFW/glfw3.h>
#include <algorithm>
#include <cmath>

// Camera base constructor
Camera::Camera(const std::string& name, const glm::vec3& position, const glm::vec3& target, const glm::vec3& up)
    : SceneObject(name), m_position(position), m_up(up), m_fov(45.0f), m_nearPlane(0.1f), m_farPlane(1000.0f),
      m_targetPosition(position), m_targetLookAt(target), m_transitioning(false) {
    if (glm::length(target - position) > 0.0001f) {
        m_front = glm::normalize(target - position);
    } else {
        m_front = glm::vec3(0.0f, 0.0f, -1.0f);
    }
}

void Camera::setTargetView(const glm::vec3& pos, const glm::vec3& lookat) {
    m_targetPosition = pos;
    m_targetLookAt = lookat;
    m_transitioning = true;
}

glm::mat4 Camera::getViewMatrix() const {
    return glm::lookAt(m_position, m_position + m_front, m_up);
}

glm::mat4 Camera::getProjectionMatrix(float aspect) const {
    return glm::perspective(glm::radians(m_fov), aspect, m_nearPlane, m_farPlane);
}

void Camera::setClipPlanes(float nearPlane, float farPlane) {
    m_nearPlane = glm::clamp(nearPlane, 0.01f, 10.0f);
    m_farPlane = std::max(farPlane, m_nearPlane + 1.0f);
}

// StaticCamera
StaticCamera::StaticCamera(const std::string& name, const glm::vec3& position, const glm::vec3& target)
    : Camera(name, position, target) {}

void StaticCamera::update(float dt) {
    if (m_transitioning) {
        m_position = glm::mix(m_position, m_targetPosition, dt * m_transitionSpeed);
        m_front = glm::normalize(glm::mix(m_position + m_front, m_targetLookAt, dt * m_transitionSpeed) - m_position);
        if (glm::distance(m_position, m_targetPosition) < 0.01f) {
            m_position = m_targetPosition;
            if (glm::length(m_targetLookAt - m_position) > 0.0001f) {
                m_front = glm::normalize(m_targetLookAt - m_position);
            }
            m_transitioning = false;
        }
    }
}

// OrbitingCamera
OrbitingCamera::OrbitingCamera(const std::string& name, float distance, float speed, float height)
    : Camera(name, glm::vec3(0.0f, height, distance), glm::vec3(0.0f)), m_distance(distance), m_speed(speed), m_height(height), m_angle(0.0f) {}

void OrbitingCamera::update(float deltaTime) {
    m_angle += m_speed * deltaTime;
    m_position.x = std::cos(m_angle) * m_distance;
    m_position.y = m_height;
    m_position.z = std::sin(m_angle) * m_distance;
    m_front = glm::normalize(glm::vec3(0.0f) - m_position); // Always look at origin
}

// TrackingCamera
TrackingCamera::TrackingCamera(const std::string& name, std::shared_ptr<Planet> target, const glm::vec3& offset)
    : Camera(name, offset, glm::vec3(0.0f)), m_targetPlanet(target), m_offset(offset) {}

void TrackingCamera::update(float deltaTime) {
    (void)deltaTime;
    if (m_targetPlanet) {
        glm::vec3 targetPos = m_targetPlanet->getPosition();
        m_position = targetPos + m_offset;
        m_front = glm::normalize(targetPos - m_position);
    }
}

// FreeCamera
FreeCamera::FreeCamera(const std::string& name, const glm::vec3& position)
    : Camera(name, position, position + glm::vec3(0.0f, 0.0f, -1.0f)), m_yaw(-90.0f), m_pitch(0.0f), m_movementSpeed(5.0f) {}

void FreeCamera::update(float deltaTime) {
    GLFWwindow* window = glfwGetCurrentContext();
    if (!window) return;
    Window* win = static_cast<Window*>(glfwGetWindowUserPointer(window));
    if (!win) return;

    // 1. Mouse look around
    double mouseDeltaX = win->getMouseDeltaX();
    double mouseDeltaY = win->getMouseDeltaY();

    float sensitivity = 0.1f;
    m_yaw   += static_cast<float>(mouseDeltaX) * sensitivity;
    m_pitch += static_cast<float>(mouseDeltaY) * sensitivity;

    // Clamp pitch to avoid screen flip
    if (m_pitch > 89.0f) m_pitch = 89.0f;
    if (m_pitch < -89.0f) m_pitch = -89.0f;

    // Update front vector based on yaw/pitch angles
    glm::vec3 direction;
    direction.x = std::cos(glm::radians(m_yaw)) * std::cos(glm::radians(m_pitch));
    direction.y = std::sin(glm::radians(m_pitch));
    direction.z = std::sin(glm::radians(m_yaw)) * std::cos(glm::radians(m_pitch));
    m_front = glm::normalize(direction);

    // Orthonormalize the up vector to keep alignment
    glm::vec3 worldUp = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 right = glm::normalize(glm::cross(m_front, worldUp));
    m_up = glm::normalize(glm::cross(right, m_front));

    // 2. Zoom via mouse scroll wheel
    double scrollDeltaY = win->getScrollDeltaY();
    if (scrollDeltaY != 0.0) {
        m_fov -= static_cast<float>(scrollDeltaY) * 2.0f;
        if (m_fov < 1.0f) m_fov = 1.0f;
        if (m_fov > 60.0f) m_fov = 60.0f;
    }

    // 3. Keyboard WASD movement
    float velocity = m_movementSpeed * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        m_position += m_front * velocity;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        m_position -= m_front * velocity;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        m_position -= right * velocity;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        m_position += right * velocity;
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) // Ascend
        m_position += m_up * velocity;
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) // Descend
        m_position -= m_up * velocity;
}

// SpacecraftFollowCamera
SpacecraftFollowCamera::SpacecraftFollowCamera(const std::string& name, std::shared_ptr<Spacecraft> target, float, float)
    : Camera(name, glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, -1.0f)), m_targetSpacecraft(target) {}

void SpacecraftFollowCamera::update(float deltaTime) {
    if (m_targetSpacecraft) {
        glm::vec3 craftPos = m_targetSpacecraft->getPosition();
        glm::vec3 craftForward = m_targetSpacecraft->getForwardDir();
        if (glm::length(craftForward) > 0.0001f) {
            craftForward = glm::normalize(craftForward);
        } else {
            craftForward = m_front;
        }

        glm::vec3 worldUp = glm::vec3(0.0f, 1.0f, 0.0f);
        glm::vec3 right = glm::cross(craftForward, worldUp);
        if (glm::length(right) < 0.0001f) {
            right = glm::cross(m_front, worldUp);
        }
        if (glm::length(right) < 0.0001f) {
            right = glm::vec3(1.0f, 0.0f, 0.0f);
        } else {
            right = glm::normalize(right);
        }

        glm::vec3 idealPos = craftPos - craftForward * 0.15f + worldUp * 0.05f + right * 0.03f;
        float alpha = glm::clamp(0.05f * deltaTime * 60.0f, 0.0f, 1.0f);
        m_position = glm::mix(m_position, idealPos, alpha);

        // Look slightly ahead of the spacecraft in its movement direction
        glm::vec3 lookAtTarget = craftPos + craftForward * 0.05f;
        glm::vec3 front = lookAtTarget - m_position;
        if (glm::length(front) > 0.0001f) {
            front = glm::normalize(front);
            m_front = glm::normalize(glm::mix(m_front, front, glm::clamp(alpha * 1.5f, 0.0f, 1.0f)));
        }

        // Keep a stable world-up horizon to avoid follow-camera roll/gimbal jitter.
        glm::vec3 viewRight = glm::cross(m_front, worldUp);
        if (glm::length(viewRight) > 0.0001f) {
            viewRight = glm::normalize(viewRight);
            m_up = glm::normalize(glm::cross(viewRight, m_front));
        } else {
            m_up = worldUp;
        }
    }
}
