#pragma once

#include "scene/SceneObject.h"
#include "core/Mesh.h"
#include <memory>

class Starfield : public SceneObject {
public:
    Starfield(const std::string& name, unsigned int count, float radius);
    virtual ~Starfield() = default;

    void update(float) override {}
    void render(Renderer& renderer) override;

private:
    std::unique_ptr<Mesh> m_mesh;
};
