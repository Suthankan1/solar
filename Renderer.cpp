#include "Renderer.h"
#include <iostream>

// Vertex Shader source code
const char* vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;

out vec3 ourColor;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    gl_Position = projection * view * model * vec4(aPos, 1.0);
    ourColor = aColor;
}
)";

// Fragment Shader source code
const char* fragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;
in vec3 ourColor;

void main() {
    FragColor = vec4(ourColor, 1.0);
}
)";

Renderer::Renderer()
    : m_shaderProgram(0), m_vao(0), m_vbo(0), m_ebo(0),
      m_modelLoc(-1), m_viewLoc(-1), m_projLoc(-1) {}

Renderer::~Renderer() {
    cleanup();
}

bool Renderer::init() {
    // 1. Compile Shaders
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    if (!compileShader(vertexShader, vertexShaderSource, "VERTEX")) {
        glDeleteShader(vertexShader);
        return false;
    }

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    if (!compileShader(fragmentShader, fragmentShaderSource, "FRAGMENT")) {
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        return false;
    }

    // 2. Link Shader Program
    m_shaderProgram = glCreateProgram();
    glAttachShader(m_shaderProgram, vertexShader);
    glAttachShader(m_shaderProgram, fragmentShader);
    
    if (!linkProgram(m_shaderProgram)) {
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        cleanup();
        return false;
    }

    // Shaders are linked into program, we can delete them
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    // 3. Get Uniform Locations
    m_modelLoc = glGetUniformLocation(m_shaderProgram, "model");
    m_viewLoc = glGetUniformLocation(m_shaderProgram, "view");
    m_projLoc = glGetUniformLocation(m_shaderProgram, "projection");

    // 4. Define Vertex Data for a colorful 3D Cube (Position + Color)
    float vertices[] = {
        // Position            // Color
        -0.5f, -0.5f,  0.5f,   1.0f, 0.0f, 0.0f, // Front-bottom-left (Red)
         0.5f, -0.5f,  0.5f,   0.0f, 1.0f, 0.0f, // Front-bottom-right (Green)
         0.5f,  0.5f,  0.5f,   0.0f, 0.0f, 1.0f, // Front-top-right (Blue)
        -0.5f,  0.5f,  0.5f,   1.0f, 1.0f, 0.0f, // Front-top-left (Yellow)
        -0.5f, -0.5f, -0.5f,   1.0f, 0.0f, 1.0f, // Back-bottom-left (Magenta)
         0.5f, -0.5f, -0.5f,   0.0f, 1.0f, 1.0f, // Back-bottom-right (Cyan)
         0.5f,  0.5f, -0.5f,   1.0f, 1.0f, 1.0f, // Back-top-right (White)
        -0.5f,  0.5f, -0.5f,   0.5f, 0.5f, 0.5f  // Back-top-left (Gray)
    };

    unsigned int indices[] = {
        0, 1, 2,  2, 3, 0, // Front face
        1, 5, 6,  6, 2, 1, // Right face
        5, 4, 7,  7, 6, 5, // Back face
        4, 0, 3,  3, 7, 4, // Left face
        3, 2, 6,  6, 7, 3, // Top face
        4, 5, 1,  1, 0, 4  // Bottom face
    };

    // 5. Generate and Bind OpenGL Buffers
    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
    glGenBuffers(1, &m_ebo);

    glBindVertexArray(m_vao);

    // VBO Setup
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // EBO Setup
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // Vertex Attributes Setup
    // Position Attribute (location = 0)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Color Attribute (location = 1)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Unbind VAO (safe practice)
    glBindVertexArray(0);

    // 6. Configure OpenGL Global States
    glEnable(GL_DEPTH_TEST);

    std::cout << "Renderer initialized successfully: Shader compiled and VAO set up.\n";
    return true;
}

void Renderer::render(int width, int height, float time) {
    // Clear the buffers
    glClearColor(0.11f, 0.12f, 0.16f, 1.0f); // Sleek modern dark slate background
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Bind shader program
    glUseProgram(m_shaderProgram);

    // Calculate Aspect Ratio (avoid division by zero)
    float aspect = (height > 0) ? (static_cast<float>(width) / static_cast<float>(height)) : 1.777f;

    // Projection matrix (45 deg FOV)
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.0f);

    // View matrix (camera positioned back and up, looking at the origin)
    glm::mat4 view = glm::lookAt(
        glm::vec3(0.0f, 1.5f, 3.0f), // Eye
        glm::vec3(0.0f, 0.0f, 0.0f), // Center
        glm::vec3(0.0f, 1.0f, 0.0f)  // Up
    );

    // Model matrix: Rotate the cube over time
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::rotate(model, time * glm::radians(30.0f), glm::vec3(1.0f, 0.0f, 0.0f)); // Rotate on X axis
    model = glm::rotate(model, time * glm::radians(45.0f), glm::vec3(0.0f, 1.0f, 0.0f)); // Rotate on Y axis

    // Send matrices to shader
    glUniformMatrix4fv(m_modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(m_viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(m_projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    // Draw Cube
    glBindVertexArray(m_vao);
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void Renderer::cleanup() {
    if (m_vao != 0) {
        glDeleteVertexArrays(1, &m_vao);
        m_vao = 0;
    }
    if (m_vbo != 0) {
        glDeleteBuffers(1, &m_vbo);
        m_vbo = 0;
    }
    if (m_ebo != 0) {
        glDeleteBuffers(1, &m_ebo);
        m_ebo = 0;
    }
    if (m_shaderProgram != 0) {
        glDeleteProgram(m_shaderProgram);
        m_shaderProgram = 0;
    }
}

bool Renderer::compileShader(GLuint shader, const char* source, const std::string& shaderType) {
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLchar infoLog[512];
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::" << shaderType << "::COMPILATION_FAILED\n" << infoLog << std::endl;
        return false;
    }
    return true;
}

bool Renderer::linkProgram(GLuint program) {
    glLinkProgram(program);

    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        GLchar infoLog[512];
        glGetProgramInfoLog(program, 512, NULL, infoLog);
        std::cerr << "ERROR::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
        return false;
    }
    return true;
}
