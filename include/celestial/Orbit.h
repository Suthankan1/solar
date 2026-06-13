#pragma once

#include "scene/SceneObject.h"
#include "core/Mesh.h"
#include <glm/glm.hpp>
#include <memory>

class Planet;

/**
 * @class Orbit
 * @brief Provides a visual representation of an orbital path as a thin tube mesh.
 */
class Orbit : public SceneObject {
public:
    Orbit(const std::string& name, float radius, const glm::vec3& color, std::shared_ptr<Planet> parentPlanet = nullptr);
    Orbit(const std::string& name, float semiMajorAxis, float semiMinorAxis, float inclination, float longitudeOfAscendingNode, const glm::vec3& color, std::shared_ptr<Planet> parentPlanet = nullptr);
    Orbit(const std::string& name, std::shared_ptr<Planet> planetToTrack, const glm::vec3& color, std::shared_ptr<Planet> parentPlanet = nullptr);
    virtual ~Orbit() = default;

    void update(float deltaTime) override;
    void render(Renderer& renderer) override;

    void setLocalOrbitStyle(float planeOffset, const glm::vec3& color, float alphaMultiplier = 1.0f);
    void setEmphasized(bool emphasized) { m_emphasized = emphasized; }
    void setHighQuality(bool enabled);

private:
    float calculateAdaptiveAlpha(const Renderer& renderer) const;
    void rebuildMesh(unsigned int segments);

    float m_semiMajorAxis;
    float m_semiMinorAxis;
    float m_inclination;
    float m_longitudeOfAscendingNode;
    glm::vec3 m_color;
    std::shared_ptr<Planet> m_parentPlanet;
    glm::vec3 m_center;
    bool m_isLocalOrbit;
    bool m_emphasized;
    bool m_highQuality;
    float m_planeOffset;
    float m_alphaMultiplier;
    std::unique_ptr<Mesh> m_mesh;
};
