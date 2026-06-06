#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <stdexcept>

class Renderer {
public:
    Renderer();
    ~Renderer();

    // Prevent copying
    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    bool init();
    void render(int width, int height, float time);
    void cleanup();

private:
    GLuint m_shaderProgram;
    GLuint m_vao;
    GLuint m_vbo;
    GLuint m_ebo;

    // Uniform locations
    GLint m_modelLoc;
    GLint m_viewLoc;
    GLint m_projLoc;

    bool compileShader(GLuint shader, const char* source, const std::string& shaderType);
    bool linkProgram(GLuint program);
};
