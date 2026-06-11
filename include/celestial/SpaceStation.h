#pragma once

#include "scene/SceneObject.h"
#include "scene/Transform.h"
#include "core/Mesh.h"
#include <glm/glm.hpp>
#include <memory>

class Planet;

/**
 * @class SpaceStation
 * @brief Represents a human-made space station orbiting a planet, assembled from simple geometries.
 */
class SpaceStation : public SceneObject {
public:
    /**
     * @brief Constructs a SpaceStation.
     * @param name Name of the station (e.g. "SpaceStation")
     * @param orbitRadius Radius of orbit around parent planet
     * @param orbitSpeed Orbital speed
     * @param parentPlanet Shared pointer to the parent planet (e.g. Earth)
     */
    SpaceStation(const std::string& name, float orbitRadius, float orbitSpeed, std::shared_ptr<Planet> parentPlanet);
    virtual ~SpaceStation() = default;

    void update(float deltaTime) override;
    void render(Renderer& renderer) override;

    glm::vec3 getPosition() const { return glm::vec3(m_transform.getModelMatrix()[3]); }
    float getOrbitRadius() const { return m_orbitRadius; }

    glm::vec3 getCloseUpTargetPoint() const;

    Transform& getTransform() { return m_transform; }
    const Transform& getTransform() const { return m_transform; }

private:
    float m_orbitRadius;
    float m_orbitSpeed;
    std::shared_ptr<Planet> m_parentPlanet;

    float m_orbitAngle;
    float m_time = 0.0f;
    Transform m_transform;

    // Shared primitive shapes to assemble the station
    std::unique_ptr<Mesh> m_cubeMesh;
    std::unique_ptr<Mesh> m_cylinderMesh;

    // Helper functions to generate unit-scale primitive meshes
    static Mesh createUnitCube();
    static Mesh createUnitCylinder(unsigned int segments = 16);
};
