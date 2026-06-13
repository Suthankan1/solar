#include "utilities/TextRenderer.h"
#include "core/Shader.h"
#include "utilities/font8x8.h"
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <iostream>
#include <iterator>

TextRenderer::TextRenderer() : m_VAO(0), m_VBO(0), m_textureID(0), m_vboCapacityBytes(0) {}

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
    m_vboCapacityBytes = sizeof(float) * 6 * 4 * 128;
    glBufferData(GL_ARRAY_BUFFER, m_vboCapacityBytes, nullptr, GL_DYNAMIC_DRAW);
    
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), nullptr);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    std::cout << "TextRenderer initialized successfully with monochrome 8x8 font atlas.\n";
    return true;
}

void TextRenderer::renderText(const std::string& text, float x, float y, float scale, const glm::vec4& color, int screenWidth, int screenHeight) {
    if (!m_shader || m_VAO == 0 || m_VBO == 0 || m_textureID == 0) return;
    if (text.empty()) return;

    // Save current OpenGL states to prevent altering main scene rendering pipeline
    GLboolean depthTestEnabled = glIsEnabled(GL_DEPTH_TEST);
    GLboolean blendEnabled = glIsEnabled(GL_BLEND);
    GLint blendSrcRgb, blendDstRgb, blendSrcAlpha, blendDstAlpha;
    glGetIntegerv(GL_BLEND_SRC_RGB, &blendSrcRgb);
    glGetIntegerv(GL_BLEND_DST_RGB, &blendDstRgb);
    glGetIntegerv(GL_BLEND_SRC_ALPHA, &blendSrcAlpha);
    glGetIntegerv(GL_BLEND_DST_ALPHA, &blendDstAlpha);

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

    std::vector<float> vertices;
    vertices.reserve(text.size() * 6 * 4);
    auto appendStringVertices = [&](float startX, float baselineY) {
        float currentX = startX;
        const float w = 8.0f * scale;
        const float h = 8.0f * scale;

        for (char c : text) {
            int charIdx = static_cast<unsigned char>(c);
            if (charIdx < 0 || charIdx >= 128) {
                charIdx = 32;
            }

            const float u0 = (charIdx * 8.0f) / 1024.0f;
            const float u1 = (charIdx * 8.0f + 8.0f) / 1024.0f;
            const float quad[] = {
                currentX,     baselineY + h, u0, 1.0f,
                currentX,     baselineY,     u0, 0.0f,
                currentX + w, baselineY,     u1, 0.0f,
                currentX,     baselineY + h, u0, 1.0f,
                currentX + w, baselineY,     u1, 0.0f,
                currentX + w, baselineY + h, u1, 1.0f
            };
            vertices.insert(vertices.end(), std::begin(quad), std::end(quad));
            currentX += w;
        }
    };

    auto drawBatch = [&](const glm::vec4& batchColor) {
        const std::size_t bytes = vertices.size() * sizeof(float);
        if (bytes > m_vboCapacityBytes) {
            m_vboCapacityBytes = bytes;
            glBufferData(GL_ARRAY_BUFFER, m_vboCapacityBytes, nullptr, GL_DYNAMIC_DRAW);
        }
        glBufferSubData(GL_ARRAY_BUFFER, 0, bytes, vertices.data());
        m_shader->setVec4("textColor", batchColor);
        glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(vertices.size() / 4));
    };

    appendStringVertices(x + 1.0f * scale, y - 1.0f * scale);
    drawBatch(glm::vec4(0.0f, 0.0f, 0.0f, color.a));

    vertices.clear();
    appendStringVertices(x, y);
    drawBatch(color);

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
    glBlendFuncSeparate(blendSrcRgb, blendDstRgb, blendSrcAlpha, blendDstAlpha);
}

void TextRenderer::renderQuad(float x, float y, float w, float h, const glm::vec4& color, int screenWidth, int screenHeight) {
    if (!m_shader || m_VAO == 0 || m_VBO == 0 || m_textureID == 0) return;

    GLboolean depthTestEnabled = glIsEnabled(GL_DEPTH_TEST);
    GLboolean blendEnabled = glIsEnabled(GL_BLEND);
    GLint blendSrcRgb, blendDstRgb, blendSrcAlpha, blendDstAlpha;
    glGetIntegerv(GL_BLEND_SRC_RGB, &blendSrcRgb);
    glGetIntegerv(GL_BLEND_DST_RGB, &blendDstRgb);
    glGetIntegerv(GL_BLEND_SRC_ALPHA, &blendSrcAlpha);
    glGetIntegerv(GL_BLEND_DST_ALPHA, &blendDstAlpha);

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
    glBlendFuncSeparate(blendSrcRgb, blendDstRgb, blendSrcAlpha, blendDstAlpha);
}
