#pragma once

#include "scene/SceneObject.h"
#include "core/Mesh.h"
#include <memory>
#include <string>

class SunHalo : public SceneObject {
public:
    SunHalo(const std::string& name, float innerRadius, float outerRadius);
    virtual ~SunHalo() = default;

    void render(Renderer& renderer) override;

private:
    std::unique_ptr<Mesh> m_halo;
};
