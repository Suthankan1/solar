#include "core/Renderer.h"
#include <GLFW/glfw3.h>
#include <iostream>
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

Renderer::Renderer()
    : m_shader(nullptr),
      m_earthShader(nullptr),
      m_sphereMesh(nullptr),
      m_lightPosition(0.0f),
      m_lightColor(2.2f, 2.0f, 1.7f),
      m_lightConstant(1.0f),
      m_lightLinear(0.007f),
      m_lightQuadratic(0.0002f) {}

Renderer::~Renderer() {
    cleanup();
}

bool Renderer::init() {
    // 1. Initialize Shader from external GLSL files
    try {
        m_shader = std::make_unique<Shader>("shaders/cube.vert", "shaders/cube.frag");
        m_bloomShader = std::make_unique<Shader>("shaders/screen_quad.vert", "shaders/hdr_bloom.frag");
        m_blurShader = std::make_unique<Shader>("shaders/screen_quad.vert", "shaders/blur.frag");
        
        try {
            m_earthShader = std::make_unique<Shader>("shaders/earth.vert", "shaders/earth.frag");
        } catch (const std::exception& e) {
            std::cerr << "Renderer Warning: Failed to load custom Earth shaders: " << e.what() << ". Falling back to standard planet shader.\n";
            m_earthShader = nullptr;
        }
    } catch (const std::exception& e) {
        std::cerr << "Renderer Init Error: Failed to load or compile shaders:\n" << e.what() << std::endl;
        return false;
    }

    // 2. Initialize default meshes for testing and demonstration
    try {
        m_sphereMesh = std::make_unique<Mesh>(createSphere(0.6f, 64, 64));
    } catch (const std::exception& e) {
        std::cerr << "Renderer Init Error: Failed to generate default meshes:\n" << e.what() << std::endl;
        return false;
    }

    // 3. Configure OpenGL Global States
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_PROGRAM_POINT_SIZE);

    initQuad();

    std::cout << "Renderer initialized successfully: Shaders and default mesh (Sphere) loaded.\n";
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
    shader.setBool("isAsteroidPointSprite", false);
    shader.setFloat("time", (float)glfwGetTime());

    mesh.draw();

    shader.setFloat("globalAlpha", 1.0f);
    shader.setInt("planetId", -1);
    shader.setBool("isAsteroidPointSprite", false);
}

/*
 * Phong lighting model: Ambient + Diffuse + Specular components combined.
 */
void Renderer::renderWithLighting(const Mesh& mesh, const Shader& shader, 
                                  const glm::mat4& model, const glm::mat4& view, const glm::mat4& projection,
                                  const glm::vec3& lightPos, const glm::vec3& lightColor, const glm::vec3& viewPos) {
    shader.use();
    shader.setMat4("model", model);
    shader.setMat4("view", view);
    shader.setMat4("projection", projection);
    
    // Set lighting parameters
    shader.setBool("useLighting", true);
    shader.setBool("isAsteroidPointSprite", false);
    shader.setVec3("lightPos", lightPos);
    shader.setVec3("lightColor", lightColor);
    shader.setVec3("viewPos", viewPos);
    shader.setFloat("ambientStrength", 0.015f);
    shader.setFloat("lightConstant", m_lightConstant);
    shader.setFloat("lightLinear", m_lightLinear);
    shader.setFloat("lightQuadratic", m_lightQuadratic);
    shader.setFloat("time", (float)glfwGetTime());

    mesh.draw();

    shader.setFloat("globalAlpha", 1.0f);
    shader.setInt("planetId", -1);
    shader.setBool("isAsteroidPointSprite", false);
}

void Renderer::render(const Mesh& mesh, const Shader& shader, const glm::mat4& model) {
    render(mesh, shader, model, m_viewMatrix, m_projMatrix);
}

/*
 * Phong lighting model: Ambient + Diffuse + Specular components combined using active scene parameters.
 */
void Renderer::renderWithLighting(const Mesh& mesh, const Shader& shader, const glm::mat4& model) {
    renderWithLighting(mesh, shader, model, m_viewMatrix, m_projMatrix, m_lightPosition, m_lightColor, m_cameraPosition);
}

void Renderer::cleanup() {
    m_sphereMesh.reset();
    m_shader.reset();
    m_earthShader.reset();
    m_bloomShader.reset();
    m_blurShader.reset();

    if (m_hdrFBO != 0) {
        glDeleteFramebuffers(1, &m_hdrFBO);
        m_hdrFBO = 0;
    }
    if (m_hdrColorBuffer != 0) {
        glDeleteTextures(1, &m_hdrColorBuffer);
        m_hdrColorBuffer = 0;
    }
    if (m_depthRBO != 0) {
        glDeleteRenderbuffers(1, &m_depthRBO);
        m_depthRBO = 0;
    }
    glDeleteFramebuffers(2, m_pingpongFBO);
    glDeleteTextures(2, m_pingpongColorBuffers);
    m_pingpongFBO[0] = m_pingpongFBO[1] = 0;
    m_pingpongColorBuffers[0] = m_pingpongColorBuffers[1] = 0;

    if (m_quadVAO != 0) {
        glDeleteVertexArrays(1, &m_quadVAO);
        m_quadVAO = 0;
    }
    if (m_quadVBO != 0) {
        glDeleteBuffers(1, &m_quadVBO);
        m_quadVBO = 0;
    }
}

void Renderer::beginFrame() {
    // Query active viewport dimensions from OpenGL directly
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    int width = viewport[2];
    int height = viewport[3];

    // Recreate/resize framebuffers if needed
    createFBOs(width, height);

    // Bind HDR Framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, m_hdrFBO);
    glViewport(0, 0, width, height);

    // Clear color and depth buffers
    glClearColor(0.04f, 0.04f, 0.06f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Renderer::endFrame(bool bloomEnabled, float vignetteStrength) {
    // Unbind HDR FBO to render to default framebuffer (or ping-pong FBOs)
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    int lastWrittenTextureIdx = 0;
    bool horizontal = true, firstPass = true;
    unsigned int amount = 10; // 5 full horizontal/vertical iterations

    if (bloomEnabled && m_blurShader) {
        m_blurShader->use();
        m_blurShader->setFloat("threshold", 0.85f);
        
        glDisable(GL_DEPTH_TEST); // No depth test for 2D screen passes
        for (unsigned int i = 0; i < amount; i++) {
            glBindFramebuffer(GL_FRAMEBUFFER, m_pingpongFBO[horizontal]);
            m_blurShader->setBool("horizontal", horizontal);
            m_blurShader->setBool("firstPass", firstPass);
            
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, firstPass ? m_hdrColorBuffer : m_pingpongColorBuffers[!horizontal]);
            m_blurShader->setInt("image", 0);
            
            renderQuad();
            
            lastWrittenTextureIdx = horizontal;
            horizontal = !horizontal;
            if (firstPass) firstPass = false;
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glEnable(GL_DEPTH_TEST);
    }

    if (m_bloomShader) {
        m_bloomShader->use();
        glDisable(GL_DEPTH_TEST);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_hdrColorBuffer);
        m_bloomShader->setInt("scene", 0);
        
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, m_pingpongColorBuffers[lastWrittenTextureIdx]);
        m_bloomShader->setInt("bloomBlur", 1);
        
        m_bloomShader->setBool("bloom", bloomEnabled);
        m_bloomShader->setFloat("exposure", 1.2f);
        m_bloomShader->setFloat("bloomIntensity", 0.8f);
        m_bloomShader->setFloat("vignetteStrength", vignetteStrength);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        renderQuad();
        glEnable(GL_DEPTH_TEST);
    }
}

void Renderer::updateFrustumPlanes(const glm::mat4& pvp) {
    // Left plane
    m_frustumPlanes[0] = glm::vec4(
        pvp[0][3] + pvp[0][0],
        pvp[1][3] + pvp[1][0],
        pvp[2][3] + pvp[2][0],
        pvp[3][3] + pvp[3][0]
    );
    // Right plane
    m_frustumPlanes[1] = glm::vec4(
        pvp[0][3] - pvp[0][0],
        pvp[1][3] - pvp[1][0],
        pvp[2][3] - pvp[2][0],
        pvp[3][3] - pvp[3][0]
    );
    // Bottom plane
    m_frustumPlanes[2] = glm::vec4(
        pvp[0][3] + pvp[0][1],
        pvp[1][3] + pvp[1][1],
        pvp[2][3] + pvp[2][1],
        pvp[3][3] + pvp[3][1]
    );
    // Top plane
    m_frustumPlanes[3] = glm::vec4(
        pvp[0][3] - pvp[0][1],
        pvp[1][3] - pvp[1][1],
        pvp[2][3] - pvp[2][1],
        pvp[3][3] - pvp[3][1]
    );
    // Near plane
    m_frustumPlanes[4] = glm::vec4(
        pvp[0][3] + pvp[0][2],
        pvp[1][3] + pvp[1][2],
        pvp[2][3] + pvp[2][2],
        pvp[3][3] + pvp[3][2]
    );
    // Far plane
    m_frustumPlanes[5] = glm::vec4(
        pvp[0][3] - pvp[0][2],
        pvp[1][3] - pvp[1][2],
        pvp[2][3] - pvp[2][2],
        pvp[3][3] - pvp[3][2]
    );

    // Normalize planes
    for (int i = 0; i < 6; ++i) {
        float length = glm::length(glm::vec3(m_frustumPlanes[i]));
        if (length > 0.0f) {
            m_frustumPlanes[i] /= length;
        }
    }
}

bool Renderer::sphereInFrustum(glm::vec3 center, float radius) {
    for (int i = 0; i < 6; ++i) {
        float dist = glm::dot(glm::vec3(m_frustumPlanes[i]), center) + m_frustumPlanes[i].w;
        if (dist < -radius) {
            return false;
        }
    }
    return true;
}

void Renderer::createFBOs(int width, int height) {
    // Avoid rebuilding if dimensions haven't changed
    if (m_fboWidth == width && m_fboHeight == height) return;

    // Delete existing resources if they exist
    if (m_hdrFBO != 0) {
        glDeleteFramebuffers(1, &m_hdrFBO);
        m_hdrFBO = 0;
    }
    if (m_hdrColorBuffer != 0) {
        glDeleteTextures(1, &m_hdrColorBuffer);
        m_hdrColorBuffer = 0;
    }
    if (m_depthRBO != 0) {
        glDeleteRenderbuffers(1, &m_depthRBO);
        m_depthRBO = 0;
    }
    if (m_pingpongFBO[0] != 0) {
        glDeleteFramebuffers(2, m_pingpongFBO);
        m_pingpongFBO[0] = m_pingpongFBO[1] = 0;
    }
    if (m_pingpongColorBuffers[0] != 0) {
        glDeleteTextures(2, m_pingpongColorBuffers);
        m_pingpongColorBuffers[0] = m_pingpongColorBuffers[1] = 0;
    }

    m_fboWidth = width;
    m_fboHeight = height;

    // Create HDR Framebuffer
    glGenFramebuffers(1, &m_hdrFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, m_hdrFBO);

    // Create HDR color texture
    glGenTextures(1, &m_hdrColorBuffer);
    glBindTexture(GL_TEXTURE_2D, m_hdrColorBuffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Create depth RBO
    glGenRenderbuffers(1, &m_depthRBO);
    glBindRenderbuffer(GL_RENDERBUFFER, m_depthRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);

    // Attach color texture and depth RBO
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_hdrColorBuffer, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_depthRBO);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "Error: HDR Framebuffer not complete!" << std::endl;
    }

    // Create ping-pong framebuffers and textures for blurring
    glGenFramebuffers(2, m_pingpongFBO);
    glGenTextures(2, m_pingpongColorBuffers);
    for (unsigned int i = 0; i < 2; ++i) {
        glBindFramebuffer(GL_FRAMEBUFFER, m_pingpongFBO[i]);
        glBindTexture(GL_TEXTURE_2D, m_pingpongColorBuffers[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_pingpongColorBuffers[i], 0);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            std::cerr << "Error: Pingpong Framebuffer " << i << " not complete!" << std::endl;
        }
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::initQuad() {
    float quadVertices[] = {
        // Positions        // Texture Coords
        -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
        -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
         1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
         1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
    };
    glGenVertexArrays(1, &m_quadVAO);
    glGenBuffers(1, &m_quadVBO);
    glBindVertexArray(m_quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
    
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glBindVertexArray(0);
}

void Renderer::renderQuad() {
    glBindVertexArray(m_quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
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
            
            vertex.texCoords = glm::vec2((float)s / (float)(sectors - 1), 1.0f - (float)r / (float)(rings - 1));

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

Mesh Renderer::createEllipticalRing(float semiMajor, float semiMinor, float inclinationDeg, float longitudeOfAscendingNodeDeg, unsigned int segments) {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    const float PI = 3.14159265359f;
    float iRad = glm::radians(inclinationDeg);
    float omegaRad = glm::radians(longitudeOfAscendingNodeDeg);

    for (unsigned int i = 0; i < segments; ++i) {
        float theta = 2.0f * PI * (float)i / (float)segments;

        // 1. Position in the orbital plane
        float x0 = semiMajor * std::cos(theta);
        float z0 = semiMinor * std::sin(theta);

        // 2. Inclination (rotate around X-axis by inclination angle)
        float x1 = x0;
        float y1 = -z0 * std::sin(iRad);
        float z1 = z0 * std::cos(iRad);

        // 3. Longitude of ascending node (rotate around Y-axis)
        float xFinal = x1 * std::cos(omegaRad) - z1 * std::sin(omegaRad);
        float yFinal = y1;
        float zFinal = x1 * std::sin(omegaRad) + z1 * std::cos(omegaRad);

        Vertex vertex;
        vertex.position = glm::vec3(xFinal, yFinal, zFinal);
        vertex.normal = glm::vec3(0.0f, 1.0f, 0.0f);
        vertex.color = glm::vec3(1.0f, 1.0f, 1.0f);
        vertex.texCoords = glm::vec2((float)i / segments, 0.0f);

        vertices.push_back(vertex);
        indices.push_back(i);
    }

    return Mesh(vertices, indices, GL_LINE_LOOP);
}
