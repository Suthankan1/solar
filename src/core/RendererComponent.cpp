#include "core/RendererComponent.h"
#include "scene/Entity.h"
#include "scene/TransformComponent.h"
#include "core/Renderer.h"
#include "core/Shader.h"
#include "core/Mesh.h"

RendererComponent::RendererComponent()
    : m_mesh(nullptr), m_shader(nullptr), m_color(1.0f), m_useLighting(true), m_shininess(32.0f) {
}

RendererComponent::RendererComponent(const Mesh* mesh, Shader* shader, const glm::vec3& color)
    : m_mesh(mesh), m_shader(shader), m_color(color), m_useLighting(true), m_shininess(32.0f) {
}

void RendererComponent::render(Renderer& renderer) {
    if (!m_mesh || !m_shader) return;

    // Retrieve the transform of the owner entity
    glm::mat4 model = glm::mat4(1.0f);
    TransformComponent* tc = m_owner->getComponent<TransformComponent>();
    if (tc) {
        model = tc->getModelMatrix();
    }

    // Bind shader and upload material properties
    m_shader->use();
    m_shader->setVec3("u_color", m_color);
    m_shader->setFloat("u_shininess", m_shininess);

    // Call Renderer draw functions
    if (m_useLighting) {
        renderer.renderWithLighting(*m_mesh, *m_shader, model);
    } else {
        renderer.render(*m_mesh, *m_shader, model);
    }
}
