#include "core/Renderer.h"
#include <iostream>
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

Renderer::Renderer() : m_shader(nullptr), m_cubeMesh(nullptr), m_sphereMesh(nullptr) {}

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
        m_cubeMesh = std::make_unique<Mesh>(createCube());
        m_sphereMesh = std::make_unique<Mesh>(createSphere(0.6f, 32, 32));
    } catch (const std::exception& e) {
        std::cerr << "Renderer Init Error: Failed to generate default meshes:\n" << e.what() << std::endl;
        return false;
    }

    // 3. Configure OpenGL Global States
    glEnable(GL_DEPTH_TEST);

    std::cout << "Renderer initialized successfully: Shaders and default meshes (Sphere, Cube) loaded.\n";
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
}

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
}

void Renderer::render(const Mesh& mesh, const Shader& shader, const glm::mat4& model) {
    render(mesh, shader, model, m_viewMatrix, m_projMatrix);
}

void Renderer::renderWithLighting(const Mesh& mesh, const Shader& shader, const glm::mat4& model) {
    renderWithLighting(mesh, shader, model, m_viewMatrix, m_projMatrix, m_lightPosition, m_lightColor, m_cameraPosition);
}

void Renderer::render(int width, int height, float time) {
    // 1. Clear the screen with a sleek modern dark slate background
    clear(glm::vec4(0.08f, 0.09f, 0.13f, 1.0f));

    if (!m_shader || !m_cubeMesh || !m_sphereMesh) return;

    // Calculate Aspect Ratio (avoid division by zero)
    float aspect = (height > 0) ? (static_cast<float>(width) / static_cast<float>(height)) : 1.777f;

    // Projection matrix (45 deg FOV)
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 1000.0f);

    // View matrix (camera positioned back and up, looking at the origin)
    glm::vec3 cameraPos(0.0f, 1.5f, 4.0f);
    glm::mat4 view = glm::lookAt(
        cameraPos,                  // Eye
        glm::vec3(0.0f, 0.0f, 0.0f), // Center
        glm::vec3(0.0f, 1.0f, 0.0f)  // Up
    );

    // Orbiting light source path calculations
    float lightRadius = 1.8f;
    glm::vec3 lightPos(
        lightRadius * std::cos(time),
        0.5f + 0.5f * std::sin(time * 0.5f), // Oscillates up and down slightly
        lightRadius * std::sin(time)
    );
    glm::vec3 lightColor(1.0f, 1.0f, 1.0f); // White light

    // --- 1. Render Central Shaded Sphere ---
    glm::mat4 modelSphere = glm::mat4(1.0f);
    // Rotate the sphere slowly on its axes over time
    modelSphere = glm::rotate(modelSphere, time * glm::radians(15.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    modelSphere = glm::rotate(modelSphere, time * glm::radians(10.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    
    renderWithLighting(*m_sphereMesh, *m_shader, modelSphere, view, projection, lightPos, lightColor, cameraPos);

    // --- 2. Render Orbiting Unlit Light Cube (represents the source of light) ---
    glm::mat4 modelLight = glm::mat4(1.0f);
    modelLight = glm::translate(modelLight, lightPos);
    modelLight = glm::scale(modelLight, glm::vec3(0.12f)); // Small cube
    
    // Pass false to lighting toggle via render() so it appears fully unlit/glowing
    render(*m_cubeMesh, *m_shader, modelLight, view, projection);
}

void Renderer::cleanup() {
    m_cubeMesh.reset();
    m_sphereMesh.reset();
    m_shader.reset();
}

Mesh Renderer::createCube() {
    // 6 faces, 4 vertices per face = 24 vertices
    std::vector<Vertex> vertices = {
        // Front Face (Normal: 0, 0, 1)
        { {-0.5f, -0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.2f, 0.2f}, {0.0f, 0.0f} }, // Reddish
        { { 0.5f, -0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {0.2f, 1.0f, 0.2f}, {1.0f, 0.0f} }, // Greenish
        { { 0.5f,  0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {0.2f, 0.2f, 1.0f}, {1.0f, 1.0f} }, // Blueish
        { {-0.5f,  0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f, 0.2f}, {0.0f, 1.0f} }, // Yellowish

        // Back Face (Normal: 0, 0, -1)
        { {-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.2f, 1.0f}, {0.0f, 0.0f} }, // Magenta
        { {-0.5f,  0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {0.6f, 0.6f, 0.6f}, {0.0f, 1.0f} }, // Gray
        { { 0.5f,  0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f} }, // White
        { { 0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {0.2f, 1.0f, 1.0f}, {1.0f, 0.0f} }, // Cyan

        // Top Face (Normal: 0, 1, 0)
        { {-0.5f,  0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f, 0.2f}, {0.0f, 0.0f} },
        { { 0.5f,  0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}, {0.2f, 0.2f, 1.0f}, {1.0f, 0.0f} },
        { { 0.5f,  0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f} },
        { {-0.5f,  0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.6f, 0.6f, 0.6f}, {0.0f, 1.0f} },

        // Bottom Face (Normal: 0, -1, 0)
        { {-0.5f, -0.5f, -0.5f}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.2f, 1.0f}, {0.0f, 0.0f} },
        { { 0.5f, -0.5f, -0.5f}, {0.0f, -1.0f, 0.0f}, {0.2f, 1.0f, 1.0f}, {1.0f, 0.0f} },
        { { 0.5f, -0.5f,  0.5f}, {0.0f, -1.0f, 0.0f}, {0.2f, 1.0f, 0.2f}, {1.0f, 1.0f} },
        { {-0.5f, -0.5f,  0.5f}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.2f, 0.2f}, {0.0f, 1.0f} },

        // Left Face (Normal: -1, 0, 0)
        { {-0.5f, -0.5f, -0.5f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 0.2f, 1.0f}, {0.0f, 0.0f} },
        { {-0.5f, -0.5f,  0.5f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 0.2f, 0.2f}, {1.0f, 0.0f} },
        { {-0.5f,  0.5f,  0.5f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 0.2f}, {1.0f, 1.0f} },
        { {-0.5f,  0.5f, -0.5f}, {-1.0f, 0.0f, 0.0f}, {0.6f, 0.6f, 0.6f}, {0.0f, 1.0f} },

        // Right Face (Normal: 1, 0, 0)
        { { 0.5f, -0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}, {0.2f, 1.0f, 0.2f}, {0.0f, 0.0f} },
        { { 0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.2f, 1.0f, 1.0f}, {1.0f, 0.0f} },
        { { 0.5f,  0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f} },
        { { 0.5f,  0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}, {0.2f, 0.2f, 1.0f}, {0.0f, 1.0f} }
    };

    std::vector<unsigned int> indices = {
        0, 1, 2,  2, 3, 0,       // Front
        4, 5, 6,  6, 7, 4,       // Back
        8, 9, 10, 10, 11, 8,     // Top
        12, 13, 14, 14, 15, 12,  // Bottom
        16, 17, 18, 18, 19, 16,  // Left
        20, 21, 22, 22, 23, 20   // Right
    };

    return Mesh(vertices, indices);
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
