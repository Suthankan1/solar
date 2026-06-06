#pragma once

#include "scene/Component.h"
#include <glm/glm.hpp>

class Mesh;
class Shader;

class RendererComponent : public Component {
public:
    RendererComponent();
    RendererComponent(const Mesh* mesh, Shader* shader, const glm::vec3& color = glm::vec3(1.0f));
    virtual ~RendererComponent() = default;

    // Overridden from Component
    void render(Renderer& renderer) override;

    // Getters and Setters
    const Mesh* getMesh() const { return m_mesh; }
    void setMesh(const Mesh* mesh) { m_mesh = mesh; }

    Shader* getShader() const { return m_shader; }
    void setShader(Shader* shader) { m_shader = shader; }

    const glm::vec3& getColor() const { return m_color; }
    void setColor(const glm::vec3& color) { m_color = color; }

    bool getUseLighting() const { return m_useLighting; }
    void setUseLighting(bool useLighting) { m_useLighting = useLighting; }

    // Material properties
    float getShininess() const { return m_shininess; }
    void setShininess(float shininess) { m_shininess = shininess; }

private:
    const Mesh* m_mesh;
    Shader* m_shader;
    glm::vec3 m_color;
    bool m_useLighting;
    float m_shininess;
};
