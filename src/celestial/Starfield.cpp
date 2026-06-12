#include "celestial/Starfield.h"
#include "core/Renderer.h"
#include <glm/gtc/matrix_transform.hpp>
#include <GLFW/glfw3.h>
#include <cstdlib>
#include <cmath>

Starfield::Starfield(const std::string& name, unsigned int count, float radius)
    : SceneObject(name), m_mesh(nullptr) {
    
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    const float PI = 3.14159265359f;

    // 1. Background Stars
    unsigned int numBackgroundStars = count * 2;
    for (unsigned int i = 0; i < numBackgroundStars; ++i) {
        // Generate uniform points on a sphere
        float theta = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX) * 2.0f * PI;
        float phi = std::acos(2.0f * (static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX)) - 1.0f);

        // Randomize depth by setting radius from base radius to 2.5x base radius
        float r = radius * (1.0f + (static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX)) * 1.5f);

        float x = std::cos(theta) * std::sin(phi) * r;
        float y = std::cos(phi) * r;
        float z = std::sin(theta) * std::sin(phi) * r;

        Vertex v;
        v.position = glm::vec3(x, y, z);
        // type: 0.0 = background star, maxAlpha = 1.0
        v.normal = glm::vec3(0.0f, 1.0f, 0.0f);
        
        // Random color selection: white, pale blue, pale orange
        float rand_color = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
        float brightness = 0.4f + (static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX)) * 0.6f;
        glm::vec3 baseCol;
        if (rand_color < 0.70f) {
            baseCol = glm::vec3(0.95f, 0.95f, 1.0f); // white
        } else if (rand_color < 0.85f) {
            baseCol = glm::vec3(0.75f, 0.85f, 1.0f); // pale blue
        } else {
            baseCol = glm::vec3(1.0f, 0.9f, 0.75f); // pale orange
        }
        v.color = baseCol * brightness;
        
        // twinkle phase in texCoords.x, size in texCoords.y
        v.texCoords.x = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
        v.texCoords.y = 1.2f + (static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX)) * 1.2f; // size: 1.2 to 2.4 pixels

        vertices.push_back(v);
        indices.push_back(vertices.size() - 1);
    }

    // 2. Galaxy Band Particles (procedural Milky Way band)
    unsigned int numGalaxyParticles = count * 1.5;
    for (unsigned int i = 0; i < numGalaxyParticles; ++i) {
        float theta = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX) * 2.0f * PI;
        
        // Concentrates galaxy particles near the galactic equator
        float offset = (static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX) - 0.5f) * 2.0f;
        float phi = PI / 2.0f + (offset * offset * offset) * 0.35f;

        // Depth variation
        float r = radius * (0.9f + (static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX)) * 1.3f);

        float x = std::cos(theta) * std::sin(phi) * r;
        float y = std::cos(phi) * r;
        float z = std::sin(theta) * std::sin(phi) * r;

        // Rotate particles to align with a tilted Milky Way plane
        glm::vec3 pos(x, y, z);
        glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), glm::radians(55.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        rotation = glm::rotate(rotation, glm::radians(30.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        pos = glm::vec3(rotation * glm::vec4(pos, 1.0f));

        Vertex v;
        v.position = pos;
        
        // Set galaxy particle attributes: type = 1.0 (galaxy), maxAlpha = 0.015 to 0.07
        float maxAlpha = 0.015f + (static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX)) * 0.055f;
        v.normal = glm::vec3(1.0f, maxAlpha, 0.0f);
        
        // Colors: orange/yellow core stars, purple/pink/blue nebulae
        float rand_color = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
        glm::vec3 col;
        if (rand_color < 0.40f) {
            col = glm::vec3(0.95f, 0.75f, 0.5f); // pale orange
        } else if (rand_color < 0.70f) {
            col = glm::vec3(0.55f, 0.25f, 0.75f); // purple/pink
        } else if (rand_color < 0.90f) {
            col = glm::vec3(0.25f, 0.5f, 0.85f); // blue/teal
        } else {
            col = glm::vec3(0.95f, 0.9f, 0.85f); // cream
        }
        float brightness = 0.6f + (static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX)) * 0.4f;
        v.color = col * brightness;
        
        v.texCoords.x = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
        v.texCoords.y = 8.0f + (static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX)) * 14.0f; // size: 8 to 22 pixels

        vertices.push_back(v);
        indices.push_back(vertices.size() - 1);
    }

    m_mesh = std::make_unique<Mesh>(vertices, indices, GL_POINTS);
}

void Starfield::render(Renderer& renderer) {
    const Shader& shader = renderer.getShader();
    shader.use();

    auto resetShaderState = [&shader]() {
        shader.setBool("useColorOverride", false);
        shader.setFloat("globalAlpha", 1.0f);
        shader.setBool("useTexture", false);
        shader.setInt("planetId", -1);
        shader.setBool("isStarfield", false);
    };

    if (!m_mesh) {
        resetShaderState();
        return;
    }

    // Apply slow parallax drift by centering the starfield at cameraPos * parallaxFactor
    glm::vec3 cameraPos = renderer.getCameraPosition();
    float parallaxFactor = 0.96f;
    glm::vec3 starfieldCenter = cameraPos * parallaxFactor;
    
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, starfieldCenter);

    shader.setBool("useColorOverride", false);
    shader.setFloat("time", (float)glfwGetTime());
    shader.setBool("isStarfield", true);
    shader.setInt("planetId", -99);

    // Disable depth write so stars always render behind everything else
    glDepthMask(GL_FALSE);

    renderer.render(*m_mesh, shader, model);

    // Re-enable depth write
    glDepthMask(GL_TRUE);

    resetShaderState();
}
