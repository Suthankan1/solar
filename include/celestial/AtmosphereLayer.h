#pragma once

#include "scene/SceneObject.h"
#include "scene/Transform.h"
#include <glm/glm.hpp>
#include <memory>

class Planet;

/**
 * @class AtmosphereLayer
 * @brief Represents a subtle atmospheric glow sphere around a planet (like Earth) using alpha blending.
 */
class AtmosphereLayer : public SceneObject {
public:
    /**
     * @brief Constructs an AtmosphereLayer relative to a target planet.
     * @param name Name of the scene object
     * @param earth Shared pointer to the parent planet (Earth)
     * @param radiusMultiplier Scaling multiplier relative to Earth's radius
     * @param color Subtle color of the atmospheric glow
     */
    AtmosphereLayer(const std::string& name, std::shared_ptr<Planet> earth, float radiusMultiplier = 1.035f, const glm::vec3& color = glm::vec3(0.25f, 0.55f, 1.0f));
    virtual ~AtmosphereLayer() = default;

    void update(float deltaTime) override;
    void render(Renderer& renderer) override;

    Transform& getTransform() { return m_transform; }
    const Transform& getTransform() const { return m_transform; }

private:
    std::shared_ptr<Planet> m_earth;
    float m_radiusMultiplier;
    glm::vec3 m_color;
    Transform m_transform;
};
