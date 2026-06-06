#pragma once

#include "scene/SceneObject.h"
#include <memory>

class Shader;

class Skybox : public SceneObject {
public:
    Skybox(const std::string& name);
    virtual ~Skybox();

    // Prevent copying
    Skybox(const Skybox&) = delete;
    Skybox& operator=(const Skybox&) = delete;

    void update(float deltaTime) override;
    void render(Renderer& renderer) override;

private:
    unsigned int m_vao;
    unsigned int m_vbo;
    unsigned int m_ebo;
    std::unique_ptr<Shader> m_shader;
    float m_time;

    void setupGeometry();
    void cleanup();
};
