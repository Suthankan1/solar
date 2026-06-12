#include "scene/Skybox.h"
#include "core/Renderer.h"
#include "core/Shader.h"
#include <glad/glad.h>
#include <iostream>

Skybox::Skybox(const std::string& name)
    : SceneObject(name), m_vao(0), m_vbo(0), m_ebo(0), m_shader(nullptr), m_time(0.0f) {
    
    // Load and compile shaders
    try {
        m_shader = std::make_unique<Shader>("shaders/skybox.vert", "shaders/skybox.frag");
    } catch (const std::exception& e) {
        std::cerr << "Skybox Init Error: Failed to compile skybox shaders:\n" << e.what() << std::endl;
        return;
    }

    setupGeometry();
}

Skybox::~Skybox() {
    cleanup();
}

void Skybox::update(float deltaTime) {
    m_time += deltaTime;
}

void Skybox::render(Renderer& renderer) {
    if (!m_shader || m_vao == 0) return;

    m_shader->use();

    // Pass the view and projection matrices
    m_shader->setMat4("view", renderer.getViewMatrix());
    m_shader->setMat4("projection", renderer.getProjectionMatrix());
    m_shader->setFloat("time", m_time);
    m_shader->setBool("useTexture", false);

    // Change depth mask/function so skybox is rendered behind everything
    glDepthMask(GL_FALSE);
    glDepthFunc(GL_LEQUAL);

    // Draw skybox cube
    glBindVertexArray(m_vao);
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);

    // Restore depth state
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);
}

void Skybox::setupGeometry() {
    float skyboxVertices[] = {
        // Positions          
        -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f,  1.0f,
         1.0f,  1.0f,  1.0f
    };

    unsigned int skyboxIndices[] = {
        // Front (z = 1)
        4, 5, 6,
        6, 7, 4,
        // Back (z = -1)
        3, 2, 1,
        1, 0, 3,
        // Left (x = -1)
        0, 1, 5,
        5, 4, 0,
        // Right (x = 1)
        7, 6, 2,
        2, 3, 7,
        // Top (y = 1)
        4, 7, 3,
        3, 0, 4,
        // Bottom (y = -1)
        1, 2, 6,
        6, 5, 1
    };

    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
    glGenBuffers(1, &m_ebo);

    glBindVertexArray(m_vao);

    // Bind VBO
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), skyboxVertices, GL_STATIC_DRAW);

    // Bind EBO
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(skyboxIndices), skyboxIndices, GL_STATIC_DRAW);

    // Vertex Attributes: Position (only layout 0 is needed for skybox)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    glBindVertexArray(0);
}

void Skybox::cleanup() {
    if (m_vbo != 0) {
        glDeleteBuffers(1, &m_vbo);
        m_vbo = 0;
    }
    if (m_ebo != 0) {
        glDeleteBuffers(1, &m_ebo);
        m_ebo = 0;
    }
    if (m_vao != 0) {
        glDeleteVertexArrays(1, &m_vao);
        m_vao = 0;
    }
}
