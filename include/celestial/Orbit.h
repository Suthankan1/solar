#pragma once

#include "scene/SceneObject.h"
#include "core/Mesh.h"
#include <glm/glm.hpp>
#include <memory>

class Planet;

/**
 * @class Orbit
 * @brief Provides a visual representation of an orbital path as a GL_LINE_LOOP ring mesh.
 */
class Orbit : public SceneObject {
public:
    Orbit(const std::string& name, float radius, const glm::vec3& color, std::shared_ptr<Planet> parentPlanet = nullptr);
    Orbit(const std::string& name, float semiMajorAxis, float semiMinorAxis, float inclination, float longitudeOfAscendingNode, const glm::vec3& color, std::shared_ptr<Planet> parentPlanet = nullptr);
    Orbit(const std::string& name, std::shared_ptr<Planet> planetToTrack, const glm::vec3& color, std::shared_ptr<Planet> parentPlanet = nullptr);
    virtual ~Orbit() = default;

    void update(float deltaTime) override;
    void render(Renderer& renderer) override;

private:
    float m_semiMajorAxis;
    float m_semiMinorAxis;
    float m_inclination;
    float m_longitudeOfAscendingNode;
    glm::vec3 m_color;
    std::shared_ptr<Planet> m_parentPlanet;
    glm::vec3 m_center;
    std::unique_ptr<Mesh> m_mesh;
};
