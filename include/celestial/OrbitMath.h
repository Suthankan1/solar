#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <cmath>

constexpr unsigned int kDefaultOrbitSegments = 720;

/**
 * Shared orbit transform used by simulation and orbit mesh generation.
 *
 * Coordinate system:
 * - World Y is up.
 * - The unrotated orbital plane is XZ, centered on the local origin.
 * - theta advances counter-clockwise through the XZ ellipse:
 *   x = semiMajor * cos(theta), z = semiMinor * sin(theta).
 *
 * Transform order:
 * 1. Build the local XZ-plane ellipse point.
 * 2. Apply inclination as a rotation around local X.
 * 3. Apply longitude of ascending node as a rotation around world/local Y.
 *
 * Callers add the orbit center after this function returns. Solar orbits add
 * (0,0,0); moon and station orbits add their parent planet's world position.
 */
inline glm::vec3 calculateOrbitPosition(float semiMajor,
                                        float semiMinor,
                                        float inclinationDeg,
                                        float nodeDeg,
                                        float theta) {
    const float iRad = glm::radians(inclinationDeg);
    const float nodeRad = glm::radians(nodeDeg);

    const float x0 = semiMajor * std::cos(theta);
    const float z0 = semiMinor * std::sin(theta);

    const float x1 = x0;
    const float y1 = -z0 * std::sin(iRad);
    const float z1 = z0 * std::cos(iRad);

    return glm::vec3(
        x1 * std::cos(nodeRad) - z1 * std::sin(nodeRad),
        y1,
        x1 * std::sin(nodeRad) + z1 * std::cos(nodeRad)
    );
}

inline glm::vec3 calculateOrbitPlaneNormal(float inclinationDeg, float nodeDeg) {
    const glm::vec3 origin = calculateOrbitPosition(1.0f, 1.0f, inclinationDeg, nodeDeg, 0.0f);
    const glm::vec3 quarter = calculateOrbitPosition(1.0f, 1.0f, inclinationDeg, nodeDeg, glm::half_pi<float>());
    glm::vec3 normal = glm::cross(origin, quarter);
    if (glm::length(normal) < 0.0001f) {
        return glm::vec3(0.0f, 1.0f, 0.0f);
    }
    return glm::normalize(normal);
}
