#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <memory>
#include "core/Shader.h"
#include "core/Mesh.h"

class Renderer {
public:
    Renderer();
    ~Renderer();

    // Prevent copying
    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    // Initialize renderer settings (depth test, etc.) and create default resources
    bool init();

    // Clear the screen with a configurable color
    void clear(const glm::vec4& color = glm::vec4(0.11f, 0.12f, 0.16f, 1.0f));

    // Setters/getters for scene parameters
    void setViewMatrix(const glm::mat4& view) { m_viewMatrix = view; }
    void setProjectionMatrix(const glm::mat4& projection) { m_projMatrix = projection; }
    void setCameraPosition(const glm::vec3& pos) { m_cameraPosition = pos; }
    void setLightSource(const glm::vec3& pos, const glm::vec3& color) {
        m_lightPosition = pos;
        m_lightColor = color;
    }
    void setLightAttenuation(float constant, float linear, float quadratic) {
        m_lightConstant = constant;
        m_lightLinear = linear;
        m_lightQuadratic = quadratic;
    }

    const glm::mat4& getViewMatrix() const { return m_viewMatrix; }
    const glm::mat4& getProjectionMatrix() const { return m_projMatrix; }
    const glm::vec3& getCameraPosition() const { return m_cameraPosition; }
    const glm::vec3& getLightPosition() const { return m_lightPosition; }
    const glm::vec3& getLightColor() const { return m_lightColor; }
    float getLightConstant() const { return m_lightConstant; }
    float getLightLinear() const { return m_lightLinear; }
    float getLightQuadratic() const { return m_lightQuadratic; }

    // Access to internal resources
    const Shader& getShader() const { return *m_shader; }
    const Shader* getEarthShader() const { return m_earthShader.get(); }
    const Mesh& getSphereMesh() const { return *m_sphereMesh; }

    // Render a mesh using a specific shader and MVP matrices
    void render(const Mesh& mesh, const Shader& shader, const glm::mat4& model, const glm::mat4& view, const glm::mat4& projection);

    // Render a mesh using a specific shader, MVP matrices, and lighting parameters
    void renderWithLighting(const Mesh& mesh, const Shader& shader, 
                            const glm::mat4& model, const glm::mat4& view, const glm::mat4& projection,
                            const glm::vec3& lightPos, const glm::vec3& lightColor, const glm::vec3& viewPos);

    // Simplified render functions utilizing internal state
    void render(const Mesh& mesh, const Shader& shader, const glm::mat4& model);
    void renderWithLighting(const Mesh& mesh, const Shader& shader, const glm::mat4& model);

    // Framebuffer-based post-processing methods
    void beginFrame();
    void endFrame(bool bloomEnabled);

    // Static shape generators to support modern OpenGL data management
    static Mesh createSphere(float radius, unsigned int rings, unsigned int sectors);
    static Mesh createRing(float radius, unsigned int segments);
    static Mesh createEllipticalRing(float semiMajor, float semiMinor, float inclinationDeg, float longitudeOfAscendingNodeDeg, unsigned int segments);

    void cleanup();

private:
    std::unique_ptr<Shader> m_shader;
    std::unique_ptr<Shader> m_earthShader;
    std::unique_ptr<Mesh> m_sphereMesh;

    // Post-processing resources
    unsigned int m_hdrFBO = 0;
    unsigned int m_hdrColorBuffer = 0;
    unsigned int m_depthRBO = 0;
    unsigned int m_pingpongFBO[2] = {0, 0};
    unsigned int m_pingpongColorBuffers[2] = {0, 0};
    int m_fboWidth = 0;
    int m_fboHeight = 0;

    unsigned int m_quadVAO = 0;
    unsigned int m_quadVBO = 0;

    std::unique_ptr<Shader> m_bloomShader;
    std::unique_ptr<Shader> m_blurShader;

    void createFBOs(int width, int height);
    void initQuad();
    void renderQuad();

    glm::mat4 m_viewMatrix = glm::mat4(1.0f);
    glm::mat4 m_projMatrix = glm::mat4(1.0f);
    glm::vec3 m_cameraPosition = glm::vec3(0.0f);
    glm::vec3 m_lightPosition = glm::vec3(0.0f);
    glm::vec3 m_lightColor = glm::vec3(1.0f);
    float m_lightConstant = 1.0f;
    float m_lightLinear = 0.05f;
    float m_lightQuadratic = 0.005f;
};
