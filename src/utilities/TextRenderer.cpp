#include "utilities/TextRenderer.h"
#include "core/Shader.h"
#include "utilities/font8x8.h"
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <iostream>

TextRenderer::TextRenderer() : m_VAO(0), m_VBO(0), m_textureID(0) {}

TextRenderer::~TextRenderer() {
    if (m_VAO) glDeleteVertexArrays(1, &m_VAO);
    if (m_VBO) glDeleteBuffers(1, &m_VBO);
    if (m_textureID) glDeleteTextures(1, &m_textureID);
}

bool TextRenderer::init() {
    // 1. Create and compile text shaders
    try {
        m_shader = std::make_unique<Shader>("shaders/text.vert", "shaders/text.frag");
    } catch (const std::exception& e) {
        std::cerr << "TextRenderer Error: Failed to compile text shaders: " << e.what() << std::endl;
        return false;
    }

    // 2. Generate Font Atlas Texture (1024 x 8 pixels, 1 channel)
    std::vector<unsigned char> pixels(1024 * 8, 0);
    for (int c = 0; c < 128; ++c) {
        for (int row = 0; row < 8; ++row) {
            unsigned char b = (c == 127) ? 255 : static_cast<unsigned char>(font8x8_basic[c][row]);
            for (int col = 0; col < 8; ++col) {
                // The LSB of the byte corresponds to the leftmost pixel
                bool pixelOn = (c == 127) ? true : ((b >> col) & 1);
                // In font8x8, row 0 is top. In OpenGL texture, y = 0 is bottom.
                int x = c * 8 + col;
                int y = 7 - row;
                pixels[y * 1024 + x] = pixelOn ? 255 : 0;
            }
        }
    }

    glGenTextures(1, &m_textureID);
    glBindTexture(GL_TEXTURE_2D, m_textureID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, 1024, 8, 0, GL_RED, GL_UNSIGNED_BYTE, pixels.data());

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);

    // 3. Setup VAO/VBO for rendering character quads (6 vertices, each with 4 floats)
    glGenVertexArrays(1, &m_VAO);
    glGenBuffers(1, &m_VBO);
    glBindVertexArray(m_VAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, nullptr, GL_DYNAMIC_DRAW);
    
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), nullptr);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    std::cout << "TextRenderer initialized successfully with monochrome 8x8 font atlas.\n";
    return true;
}

void TextRenderer::renderText(const std::string& text, float x, float y, float scale, const glm::vec4& color, int screenWidth, int screenHeight) {
    if (!m_shader || m_VAO == 0 || m_VBO == 0 || m_textureID == 0) return;

    // Save current OpenGL states to prevent altering main scene rendering pipeline
    GLboolean depthTestEnabled = glIsEnabled(GL_DEPTH_TEST);
    GLboolean blendEnabled = glIsEnabled(GL_BLEND);
    GLint blendSrc, blendDst;
    glGetIntegerv(GL_BLEND_SRC_ALPHA, &blendSrc);
    glGetIntegerv(GL_BLEND_DST_ALPHA, &blendDst);

    // Configure states for 2D UI overlay text rendering
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    m_shader->use();
    m_shader->setVec4("textColor", color);
    
    glm::mat4 projection = glm::ortho(0.0f, static_cast<float>(screenWidth), 0.0f, static_cast<float>(screenHeight));
    m_shader->setMat4("projection", projection);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_textureID);
    m_shader->setInt("text", 0);

    glBindVertexArray(m_VAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);

    // Draw shadow first
    float shadowOffset = 1.0f * scale;
    m_shader->setVec4("textColor", glm::vec4(0.0f, 0.0f, 0.0f, color.a));
    float currentX = x + shadowOffset;
    float shadowY = y - shadowOffset;
    for (char c : text) {
        int charIdx = static_cast<unsigned char>(c);
        if (charIdx < 0 || charIdx >= 128) {
            charIdx = 32; // Default to space
        }

        // Texture atlas coordinates: width is 1024, each glyph spans 8 pixels
        float u0 = (charIdx * 8.0f) / 1024.0f;
        float u1 = (charIdx * 8.0f + 8.0f) / 1024.0f;

        float w = 8.0f * scale;
        float h = 8.0f * scale;

        // Triangles quad representation: position.xy, texcoords.zw
        float vertices[6][4] = {
            { currentX,     shadowY + h,   u0, 1.0f },
            { currentX,     shadowY,       u0, 0.0f },
            { currentX + w, shadowY,       u1, 0.0f },

            { currentX,     shadowY + h,   u0, 1.0f },
            { currentX + w, shadowY,       u1, 0.0f },
            { currentX + w, shadowY + h,   u1, 1.0f }
        };

        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        // Advance cursor position
        currentX += w;
    }

    // Now draw colored text on top
    m_shader->setVec4("textColor", color);
    currentX = x;
    for (char c : text) {
        int charIdx = static_cast<unsigned char>(c);
        if (charIdx < 0 || charIdx >= 128) {
            charIdx = 32; // Default to space
        }

        // Texture atlas coordinates: width is 1024, each glyph spans 8 pixels
        float u0 = (charIdx * 8.0f) / 1024.0f;
        float u1 = (charIdx * 8.0f + 8.0f) / 1024.0f;

        float w = 8.0f * scale;
        float h = 8.0f * scale;

        // Triangles quad representation: position.xy, texcoords.zw
        float vertices[6][4] = {
            { currentX,     y + h,   u0, 1.0f },
            { currentX,     y,       u0, 0.0f },
            { currentX + w, y,       u1, 0.0f },

            { currentX,     y + h,   u0, 1.0f },
            { currentX + w, y,       u1, 0.0f },
            { currentX + w, y + h,   u1, 1.0f }
        };

        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        // Advance cursor position
        currentX += w;
    }

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);

    // Restore previous OpenGL state
    if (depthTestEnabled) {
        glEnable(GL_DEPTH_TEST);
    } else {
        glDisable(GL_DEPTH_TEST);
    }
    
    if (blendEnabled) {
        glEnable(GL_BLEND);
    } else {
        glDisable(GL_BLEND);
    }
    glBlendFunc(blendSrc, blendDst);
}

void TextRenderer::renderQuad(float x, float y, float w, float h, const glm::vec4& color, int screenWidth, int screenHeight) {
    if (!m_shader || m_VAO == 0 || m_VBO == 0 || m_textureID == 0) return;

    GLboolean depthTestEnabled = glIsEnabled(GL_DEPTH_TEST);
    GLboolean blendEnabled = glIsEnabled(GL_BLEND);
    GLint blendSrc, blendDst;
    glGetIntegerv(GL_BLEND_SRC_ALPHA, &blendSrc);
    glGetIntegerv(GL_BLEND_DST_ALPHA, &blendDst);

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    m_shader->use();
    m_shader->setVec4("textColor", color);
    
    glm::mat4 projection = glm::ortho(0.0f, static_cast<float>(screenWidth), 0.0f, static_cast<float>(screenHeight));
    m_shader->setMat4("projection", projection);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_textureID);
    m_shader->setInt("text", 0);

    glBindVertexArray(m_VAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);

    // Using character 127 (which we set to solid white)
    float u0 = (127 * 8.0f) / 1024.0f;
    float u1 = (127 * 8.0f + 8.0f) / 1024.0f;

    float vertices[6][4] = {
        { x,     y + h,   u0, 1.0f },
        { x,     y,       u0, 0.0f },
        { x + w, y,       u1, 0.0f },

        { x,     y + h,   u0, 1.0f },
        { x + w, y,       u1, 0.0f },
        { x + w, y + h,   u1, 1.0f }
    };

    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);

    if (depthTestEnabled) glEnable(GL_DEPTH_TEST);
    else glDisable(GL_DEPTH_TEST);
    
    if (blendEnabled) glEnable(GL_BLEND);
    else glDisable(GL_BLEND);
    glBlendFunc(blendSrc, blendDst);
}
