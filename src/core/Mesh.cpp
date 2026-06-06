#include "core/Mesh.h"
#include <cstddef>
#include <utility>

Mesh::Mesh(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices, unsigned int drawMode)
    : vertices(vertices), indices(indices), VAO(0), VBO(0), EBO(0), drawMode(drawMode) {
    setupMesh();
}

Mesh::~Mesh() {
    cleanup();
}

// Move constructor
Mesh::Mesh(Mesh&& other) noexcept
    : vertices(std::move(other.vertices)),
      indices(std::move(other.indices)),
      VAO(other.VAO),
      VBO(other.VBO),
      EBO(other.EBO),
      drawMode(other.drawMode) {
    other.VAO = 0;
    other.VBO = 0;
    other.EBO = 0;
    other.drawMode = GL_TRIANGLES;
}

// Move assignment operator
Mesh& Mesh::operator=(Mesh&& other) noexcept {
    if (this != &other) {
        cleanup();
        
        vertices = std::move(other.vertices);
        indices = std::move(other.indices);
        VAO = other.VAO;
        VBO = other.VBO;
        EBO = other.EBO;
        drawMode = other.drawMode;

        other.VAO = 0;
        other.VBO = 0;
        other.EBO = 0;
        other.drawMode = GL_TRIANGLES;
    }
    return *this;
}

void Mesh::setupMesh() {
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);
    
    // Upload vertices
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);

    // Upload indices
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

    // Vertex Attributes:
    // 1. Position (location = 0)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
    
    // 2. Normal (location = 1)
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));

    // 3. Color (location = 2)
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, color));

    // 4. TexCoords (location = 3)
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoords));

    glBindVertexArray(0);
}

void Mesh::draw() const {
    if (VAO == 0) return;
    glBindVertexArray(VAO);
    glDrawElements(drawMode, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void Mesh::cleanup() {
    if (VAO != 0) {
        glDeleteVertexArrays(1, &VAO);
        VAO = 0;
    }
    if (VBO != 0) {
        glDeleteBuffers(1, &VBO);
        VBO = 0;
    }
    if (EBO != 0) {
        glDeleteBuffers(1, &EBO);
        EBO = 0;
    }
}
