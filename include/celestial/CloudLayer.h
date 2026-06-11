#pragma once

#include "scene/SceneObject.h"
#include "scene/Transform.h"
#include <glm/glm.hpp>
#include <memory>

class Planet;

/**
 * @class CloudLayer
 * @brief Represents a transparent, rotating cloud sphere overlaid on a planet (like Earth).
 */
class CloudLayer : public SceneObject {
public:
    /**
     * @brief Constructs a CloudLayer relative to a target planet.
     * @param name Name of the scene object
     * @param earth Shared pointer to the parent planet (Earth)
     * @param radiusMultiplier Scaling multiplier relative to Earth's radius
     * @param rotationSpeed Self-rotation speed
     */
    CloudLayer(const std::string& name, std::shared_ptr<Planet> earth, float radiusMultiplier = 1.015f, float rotationSpeed = 1.95f);
    virtual ~CloudLayer() = default;

    void update(float deltaTime) override;
    void render(Renderer& renderer) override;

    Transform& getTransform() { return m_transform; }
    const Transform& getTransform() const { return m_transform; }

private:
    std::shared_ptr<Planet> m_earth;
    float m_radiusMultiplier;
    float m_rotationSpeed;
    float m_rotationAngle;
    Transform m_transform;
};
