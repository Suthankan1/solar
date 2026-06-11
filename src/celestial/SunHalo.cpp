#include "celestial/SunHalo.h"
#include "core/Renderer.h"
#include "core/Shader.h"
#include <glad/glad.h>
#include <cmath>
#include <vector>

SunHalo::SunHalo(const std::string& name, float innerRadius, float outerRadius)
    : SceneObject(name) {
    const float PI = 3.14159265359f;
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    for (unsigned int i = 0; i <= 128; ++i) {
        float angle = 2.0f * PI * static_cast<float>(i) / 128.0f;
        float cosA = std::cos(angle);
        float sinA = std::sin(angle);

        // Inner vertex
        Vertex inner;
        inner.position = glm::vec3(cosA * innerRadius, 0.0f, sinA * innerRadius);
        inner.normal = glm::vec3(0.0f, 1.0f, 0.0f);
        inner.color = glm::vec3(1.0f, 0.7f, 0.2f);
        inner.texCoords = glm::vec2(static_cast<float>(i) / 128.0f, 0.0f);
        vertices.push_back(inner);

        // Outer vertex
        Vertex outer;
        outer.position = glm::vec3(cosA * outerRadius, 0.0f, sinA * outerRadius);
        outer.normal = glm::vec3(0.0f, 1.0f, 0.0f);
        outer.color = glm::vec3(1.0f, 0.7f, 0.2f);
        outer.texCoords = glm::vec2(static_cast<float>(i) / 128.0f, 1.0f);
        vertices.push_back(outer);

        indices.push_back(2 * i);
        indices.push_back(2 * i + 1);
    }

    m_halo = std::make_unique<Mesh>(vertices, indices, GL_TRIANGLE_STRIP);
}

void SunHalo::render(Renderer& renderer) {
    if (!m_halo) return;

    // Enable additive blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    // Disable depth write
    glDepthMask(GL_FALSE);

    const Shader& shader = renderer.getShader();
    shader.use();

    // Set useColorOverride=true, colorOverride = glm::vec3(1.0f, 0.55f, 0.05f)
    shader.setBool("useColorOverride", true);
    shader.setVec3("colorOverride", glm::vec3(1.0f, 0.55f, 0.05f));

    // Set emissiveStrength uniform to 1.2f
    shader.setFloat("emissiveStrength", 1.2f);
    shader.setInt("planetId", 100); // Unique ID for SunHalo

    // Compute billboarding model matrix centered at the origin
    glm::mat4 view = renderer.getViewMatrix();
    glm::mat4 model = glm::mat4(1.0f);
    
    // Column 0 matches local x to Camera Right
    model[0] = glm::vec4(view[0][0], view[1][0], view[2][0], 0.0f);
    // Column 1 matches local y (plane normal) to Camera -Forward
    model[1] = glm::vec4(-view[0][2], -view[1][2], -view[2][2], 0.0f);
    // Column 2 matches local z to Camera Up
    model[2] = glm::vec4(view[0][1], view[1][1], view[2][1], 0.0f);
    // Column 3 is translation (centered at the Sun's position, i.e., (0,0,0))
    model[3] = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);

    // Render unlit (useLighting=false)
    renderer.render(*m_halo, shader, model);

    // Re-enable depth write
    glDepthMask(GL_TRUE);

    // Restore normal blending
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_BLEND);

    // Reset shader states
    shader.setBool("useColorOverride", false);
    shader.setFloat("emissiveStrength", 0.0f);
    shader.setInt("planetId", -1);
}
