#pragma once

#include "scene/SceneObject.h"
#include "scene/Transform.h"
#include "core/Mesh.h"
#include <glm/glm.hpp>
#include <memory>
#include <vector>

class Planet;

/**
 * @class Spacecraft
 * @brief Represents a realistic astronaut spacecraft traveling from Earth to Mars.
 */
class Spacecraft : public SceneObject {
public:
    /**
     * @brief Constructs a Spacecraft.
     * @param name Name of the spacecraft (e.g. "Spacecraft")
     * @param earth Shared pointer to Earth
     * @param mars Shared pointer to Mars
     */
    Spacecraft(const std::string& name, std::shared_ptr<Planet> earth, std::shared_ptr<Planet> mars);
    virtual ~Spacecraft() = default;

    void update(float deltaTime) override;
    void render(Renderer& renderer) override;

    // Getters for camera follow and HUD telemetry
    glm::vec3 getPosition() const { return m_position; }
    glm::vec3 getForwardDir() const { return m_forward; }
    float getProgress() const { return m_progress; }

    Transform& getTransform() { return m_transform; }
    const Transform& getTransform() const { return m_transform; }

private:
    std::shared_ptr<Planet> m_earth;
    std::shared_ptr<Planet> m_mars;
    float m_progress; // ranges from 0.0f (Earth) to 1.0f (Mars)
    bool m_reversed = false; // true while returning from Mars to Earth
    float m_arrivalFlash = 0.0f;
    float m_rollAngle; // subtle roll rotation (barbecue roll)

    glm::vec3 m_position;
    glm::vec3 m_forward;
    Transform m_transform;

    // Primitives to assemble the spacecraft
    std::unique_ptr<Mesh> m_cubeMesh;
    std::unique_ptr<Mesh> m_cylinderMesh;
    std::unique_ptr<Mesh> m_coneMesh;

    // Trajectory path line rendering
    std::unique_ptr<Mesh> m_trajectoryMesh;
    std::vector<Vertex> m_trajVertices;

    // Curve evaluation helpers
    glm::vec3 getPositionForT(float t) const;
    glm::vec3 getTangentForT(float t) const;
    glm::vec3 getPlanetPositionAtAngle(std::shared_ptr<Planet> planet, float angle) const;

    // Primitive generators
    static Mesh createUnitCube();
    static Mesh createUnitCylinder(unsigned int segments = 16);
    static Mesh createUnitCone(unsigned int segments = 16);
};
