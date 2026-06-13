#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <string>
#include <memory>
#include <cstddef>

class Shader;

class TextRenderer {
public:
    TextRenderer();
    ~TextRenderer();

    // Prevent copying
    TextRenderer(const TextRenderer&) = delete;
    TextRenderer& operator=(const TextRenderer&) = delete;

    bool init();
    void renderText(const std::string& text, float x, float y, float scale, const glm::vec3& color, int screenWidth, int screenHeight) {
        renderText(text, x, y, scale, glm::vec4(color, 1.0f), screenWidth, screenHeight);
    }
    void renderText(const std::string& text, float x, float y, float scale, const glm::vec4& color, int screenWidth, int screenHeight);
    void renderQuad(float x, float y, float w, float h, const glm::vec4& color, int screenWidth, int screenHeight);

private:
    std::unique_ptr<Shader> m_shader;
    GLuint m_VAO;
    GLuint m_VBO;
    GLuint m_textureID;
    std::size_t m_vboCapacityBytes;
};
