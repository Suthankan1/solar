#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec3 color;
    glm::vec2 texCoords;
};

class Mesh {
public:
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    unsigned int VAO;
    unsigned int VBO;
    unsigned int EBO;
    unsigned int drawMode;

    Mesh(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices, unsigned int drawMode = GL_TRIANGLES);
    ~Mesh();

    // Support move semantics (transfer ownership of buffers)
    Mesh(Mesh&& other) noexcept;
    Mesh& operator=(Mesh&& other) noexcept;

    // Disable copy semantics to prevent double deletion of OpenGL buffers
    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;

    void draw() const;
    void cleanup();

private:
    void setupMesh();
};
