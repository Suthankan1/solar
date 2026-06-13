#pragma once

#include "scene/SceneObject.h"
#include "scene/Transform.h"
#include "core/Mesh.h"
#include <memory>
#include <string>

class Planet;

class SaturnRings : public SceneObject {
public:
    static constexpr float kInnerRadiusMultiplier = 1.38f;
    static constexpr float kMiddleRadiusMultiplier = 1.75f;
    static constexpr float kOuterRadiusMultiplier = 2.25f;

    SaturnRings(const std::string& name, std::shared_ptr<Planet> parent);
    virtual ~SaturnRings() = default;

    void update(float deltaTime) override;
    void render(Renderer& renderer) override;

private:
    std::shared_ptr<Planet> m_parent;
    std::unique_ptr<Mesh> m_innerRing;
    std::unique_ptr<Mesh> m_middleRing;
    std::unique_ptr<Mesh> m_outerRing;
    Transform m_transform;
};
