#include "core/Renderer.h"
#include <iostream>
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

Renderer::Renderer() : m_shader(nullptr), m_sphereMesh(nullptr) {}

Renderer::~Renderer() {
    cleanup();
}

bool Renderer::init() {
    // 1. Initialize Shader from external GLSL files
    try {
        m_shader = std::make_unique<Shader>("shaders/cube.vert", "shaders/cube.frag");
    } catch (const std::exception& e) {
        std::cerr << "Renderer Init Error: Failed to load or compile shaders:\n" << e.what() << std::endl;
        return false;
    }

    // 2. Initialize default meshes for testing and demonstration
    try {
        m_sphereMesh = std::make_unique<Mesh>(createSphere(0.6f, 64, 64));
    } catch (const std::exception& e) {
        std::cerr << "Renderer Init Error: Failed to generate default meshes:\n" << e.what() << std::endl;
        return false;
    }

    // 3. Configure OpenGL Global States
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_PROGRAM_POINT_SIZE);

    std::cout << "Renderer initialized successfully: Shaders and default mesh (Sphere) loaded.\n";
    return true;
}

void Renderer::clear(const glm::vec4& color) {
    glClearColor(color.r, color.g, color.b, color.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Renderer::render(const Mesh& mesh, const Shader& shader, const glm::mat4& model, const glm::mat4& view, const glm::mat4& projection) {
    shader.use();
    shader.setMat4("model", model);
    shader.setMat4("view", view);
    shader.setMat4("projection", projection);
    shader.setBool("useLighting", false);

    mesh.draw();

    shader.setFloat("globalAlpha", 1.0f);
}

/*
 * Phong lighting model: Ambient + Diffuse + Specular components combined.
 */
void Renderer::renderWithLighting(const Mesh& mesh, const Shader& shader, 
                                  const glm::mat4& model, const glm::mat4& view, const glm::mat4& projection,
                                  const glm::vec3& lightPos, const glm::vec3& lightColor, const glm::vec3& viewPos) {
    shader.use();
    shader.setMat4("model", model);
    shader.setMat4("view", view);
    shader.setMat4("projection", projection);
    
    // Set lighting parameters
    shader.setBool("useLighting", true);
    shader.setVec3("lightPos", lightPos);
    shader.setVec3("lightColor", lightColor);
    shader.setVec3("viewPos", viewPos);

    mesh.draw();

    shader.setFloat("globalAlpha", 1.0f);
}

void Renderer::render(const Mesh& mesh, const Shader& shader, const glm::mat4& model) {
    render(mesh, shader, model, m_viewMatrix, m_projMatrix);
}

/*
 * Phong lighting model: Ambient + Diffuse + Specular components combined using active scene parameters.
 */
void Renderer::renderWithLighting(const Mesh& mesh, const Shader& shader, const glm::mat4& model) {
    renderWithLighting(mesh, shader, model, m_viewMatrix, m_projMatrix, m_lightPosition, m_lightColor, m_cameraPosition);
}

void Renderer::cleanup() {
    m_sphereMesh.reset();
    m_shader.reset();
}

Mesh Renderer::createSphere(float radius, unsigned int rings, unsigned int sectors) {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    const float PI = 3.14159265359f;

    for (unsigned int r = 0; r < rings; ++r) {
        float phi = PI * (float)r / (float)(rings - 1); // [0, PI]
        for (unsigned int s = 0; s < sectors; ++s) {
            float theta = 2.0f * PI * (float)s / (float)(sectors - 1); // [0, 2PI]

            float x = std::cos(theta) * std::sin(phi);
            float y = std::cos(phi);
            float z = std::sin(theta) * std::sin(phi);

            Vertex vertex;
            vertex.position = glm::vec3(x, y, z) * radius;
            vertex.normal = glm::vec3(x, y, z); // Normal for unit sphere is just position normalized

            // Create a gorgeous neon cyan-blue-purple procedural gradient
            vertex.color = glm::vec3(
                0.2f + 0.8f * (x * 0.5f + 0.5f), 
                0.3f + 0.5f * (y * 0.5f + 0.5f), 
                0.9f + 0.1f * (z * 0.5f + 0.5f)
            );
            
            vertex.texCoords = glm::vec2((float)s / (float)(sectors - 1), (float)r / (float)(rings - 1));

            vertices.push_back(vertex);
        }
    }

    for (unsigned int r = 0; r < rings - 1; ++r) {
        for (unsigned int s = 0; s < sectors - 1; ++s) {
            unsigned int first = r * sectors + s;
            unsigned int second = first + 1;
            unsigned int third = (r + 1) * sectors + s;
            unsigned int fourth = third + 1;

            // CCW triangles
            indices.push_back(first);
            indices.push_back(third);
            indices.push_back(second);

            indices.push_back(second);
            indices.push_back(third);
            indices.push_back(fourth);
        }
    }

    return Mesh(vertices, indices);
}

Mesh Renderer::createRing(float radius, unsigned int segments) {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    const float PI = 3.14159265359f;
    for (unsigned int i = 0; i < segments; ++i) {
        float angle = 2.0f * PI * (float)i / (float)segments;
        float x = std::cos(angle) * radius;
        float z = std::sin(angle) * radius;

        Vertex vertex;
        vertex.position = glm::vec3(x, 0.0f, z);
        vertex.normal = glm::vec3(0.0f, 1.0f, 0.0f);
        vertex.color = glm::vec3(1.0f, 1.0f, 1.0f); // Default to white
        vertex.texCoords = glm::vec2((float)i / segments, 0.0f);

        vertices.push_back(vertex);
        indices.push_back(i);
    }

    return Mesh(vertices, indices, GL_LINE_LOOP);
}
