#include "celestial/SunHalo.h"
#include "core/Renderer.h"
#include "core/Shader.h"
#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>
#include <array>
#include <cmath>
#include <vector>

SunHalo::SunHalo(const std::string& name, float innerRadius, float outerRadius)
    : SceneObject(name) {
    const float PI = 3.14159265359f;
    const std::array<float, 3> layerRadii = {1.1f, 1.4f, 1.8f};

    for (float radiusScale : layerRadii) {
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;
        float layerOuterRadius = innerRadius * radiusScale;

        for (unsigned int i = 0; i <= 128; ++i) {
            float angle = 2.0f * PI * static_cast<float>(i) / 128.0f;
            float cosA = std::cos(angle);
            float sinA = std::sin(angle);

            Vertex inner;
            inner.position = glm::vec3(cosA * innerRadius, 0.0f, sinA * innerRadius);
            inner.normal = glm::vec3(0.0f, 1.0f, 0.0f);
            inner.color = glm::vec3(1.0f, 0.7f, 0.2f);
            inner.texCoords = glm::vec2(static_cast<float>(i) / 128.0f, 0.0f);
            vertices.push_back(inner);

            Vertex outer;
            outer.position = glm::vec3(cosA * layerOuterRadius, 0.0f, sinA * layerOuterRadius);
            outer.normal = glm::vec3(0.0f, 1.0f, 0.0f);
            outer.color = glm::vec3(1.0f, 0.7f, 0.2f);
            outer.texCoords = glm::vec2(static_cast<float>(i) / 128.0f, 1.0f);
            vertices.push_back(outer);

            indices.push_back(2 * i);
            indices.push_back(2 * i + 1);
        }

        m_haloLayers.push_back(std::make_unique<Mesh>(vertices, indices, GL_TRIANGLE_STRIP));
    }

    (void)outerRadius;

    std::vector<Vertex> flareVertices(4);
    flareVertices[0].position = glm::vec3(-0.5f, 0.0f,  0.5f);
    flareVertices[1].position = glm::vec3(-0.5f, 0.0f, -0.5f);
    flareVertices[2].position = glm::vec3( 0.5f, 0.0f,  0.5f);
    flareVertices[3].position = glm::vec3( 0.5f, 0.0f, -0.5f);
    flareVertices[0].texCoords = glm::vec2(0.0f, 1.0f);
    flareVertices[1].texCoords = glm::vec2(0.0f, 0.0f);
    flareVertices[2].texCoords = glm::vec2(1.0f, 1.0f);
    flareVertices[3].texCoords = glm::vec2(1.0f, 0.0f);

    for (Vertex& vertex : flareVertices) {
        vertex.normal = glm::vec3(0.0f, 1.0f, 0.0f);
        vertex.color = glm::vec3(1.0f, 0.7f, 0.2f);
    }

    m_flareQuad = std::make_unique<Mesh>(flareVertices, std::vector<unsigned int>{0, 1, 2, 3}, GL_TRIANGLE_STRIP);
}

void SunHalo::render(Renderer& renderer) {
    const Shader& shader = renderer.getShader();
    shader.use();

    auto resetShaderState = [&shader]() {
        shader.setBool("useColorOverride", false);
        shader.setFloat("globalAlpha", 1.0f);
        shader.setBool("useTexture", false);
        shader.setInt("planetId", -1);
        shader.setFloat("emissiveStrength", 0.0f);
    };

    if (m_haloLayers.empty()) {
        resetShaderState();
        return;
    }

    // Enable additive blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    // Disable depth write
    glDepthMask(GL_FALSE);

    // Set useColorOverride=true, colorOverride = glm::vec3(1.0f, 0.55f, 0.05f)
    shader.setBool("useColorOverride", true);
    shader.setVec3("colorOverride", glm::vec3(1.0f, 0.55f, 0.05f));
    shader.setBool("useTexture", false);

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
    const std::array<float, 3> layerAlpha = {0.08f, 0.05f, 0.03f};
    for (std::size_t i = 0; i < m_haloLayers.size() && i < layerAlpha.size(); ++i) {
        shader.setFloat("globalAlpha", layerAlpha[i]);
        renderer.render(*m_haloLayers[i], shader, model);
        shader.setInt("planetId", 100);
    }

    glm::vec3 cameraPos = renderer.getCameraPosition();
    glm::vec3 sunDir = glm::normalize(-cameraPos);
    glm::vec3 cameraForward = glm::normalize(glm::vec3(-view[0][2], -view[1][2], -view[2][2]));
    float sunAlignment = glm::dot(cameraForward, sunDir);

    if (m_flareQuad && sunAlignment > 0.94f) {
        glm::vec4 sunClip = renderer.getProjectionMatrix() * view * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
        if (sunClip.w > 0.0f) {
            glm::vec2 sunNdc = glm::vec2(sunClip) / sunClip.w;
            glm::vec3 cameraRight = glm::normalize(glm::vec3(view[0][0], view[1][0], view[2][0]));
            glm::vec3 cameraUp = glm::normalize(glm::vec3(view[0][1], view[1][1], view[2][1]));
            glm::vec2 lineDir = -sunNdc;
            if (glm::length(lineDir) < 0.001f) {
                lineDir = glm::vec2(1.0f, 0.0f);
            } else {
                lineDir = glm::normalize(lineDir);
            }

            float flareStrength = glm::smoothstep(0.94f, 0.995f, sunAlignment);
            float planeDistance = 1.2f;
            float planeHalfHeight = 0.7f;
            float planeHalfWidth = planeHalfHeight;
            const std::array<float, 4> offsets = {0.25f, 0.48f, 0.72f, 0.95f};
            const std::array<float, 4> sizes = {0.055f, 0.035f, 0.045f, 0.025f};
            const std::array<glm::vec3, 4> colors = {
                glm::vec3(1.0f, 0.72f, 0.22f),
                glm::vec3(1.0f, 0.92f, 0.56f),
                glm::vec3(1.0f, 0.48f, 0.18f),
                glm::vec3(0.95f, 0.70f, 0.35f)
            };

            shader.setInt("planetId", 101);
            shader.setFloat("emissiveStrength", 1.8f);

            for (std::size_t i = 0; i < offsets.size(); ++i) {
                glm::vec2 flareNdc = sunNdc + lineDir * offsets[i];
                glm::vec3 flareCenter = cameraPos + cameraForward * planeDistance
                    + cameraRight * flareNdc.x * planeHalfWidth
                    + cameraUp * flareNdc.y * planeHalfHeight;

                glm::mat4 flareModel(1.0f);
                flareModel[0] = glm::vec4(cameraRight * sizes[i], 0.0f);
                flareModel[1] = glm::vec4(cameraForward * sizes[i], 0.0f);
                flareModel[2] = glm::vec4(cameraUp * sizes[i], 0.0f);
                flareModel[3] = glm::vec4(flareCenter, 1.0f);

                shader.setVec3("colorOverride", colors[i]);
                shader.setFloat("globalAlpha", flareStrength * (0.18f - 0.025f * static_cast<float>(i)));
                renderer.render(*m_flareQuad, shader, flareModel);
                shader.setInt("planetId", 101);
            }
        }
    }

    // Re-enable depth write
    glDepthMask(GL_TRUE);

    // Restore normal blending
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_BLEND);

    // Reset shader states
    resetShaderState();
}
