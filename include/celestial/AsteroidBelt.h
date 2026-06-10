#pragma once

#include "scene/SceneObject.h"
#include "scene/Transform.h"
#include "core/Mesh.h"
#include <memory>
#include <string>

class AsteroidBelt : public SceneObject {
public:
    AsteroidBelt(const std::string& name, float innerRadius, float outerRadius, unsigned int count);
    virtual ~AsteroidBelt() = default;

    void update(float deltaTime) override;
    void render(Renderer& renderer) override;

private:
    std::unique_ptr<Mesh> m_mesh;
    Transform m_transform;
    float m_angle = 0.0f;
};
