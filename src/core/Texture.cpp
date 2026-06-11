#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "core/Texture.h"
#include <glad/glad.h>
#include <iostream>

Texture::Texture() : m_id(0) {}

Texture::~Texture() {
    cleanup();
}

Texture::Texture(Texture&& other) noexcept : m_id(other.m_id) {
    other.m_id = 0;
}

Texture& Texture::operator=(Texture&& other) noexcept {
    if (this != &other) {
        cleanup();
        m_id = other.m_id;
        other.m_id = 0;
    }
    return *this;
}

bool Texture::load(const std::string& path) {
    cleanup();

    int width, height, nrChannels;
    // Flip textures vertically on load so they map correctly to OpenGL's coordinate system
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(path.c_str(), &width, &height, &nrChannels, 0);
    if (!data) {
        std::cerr << "Texture load error: Failed to load texture from path: " << path << std::endl;
        std::cerr << "stb_image reason: " << stbi_failure_reason() << std::endl;
        return false;
    }

    glGenTextures(1, &m_id);
    glBindTexture(GL_TEXTURE_2D, m_id);

    // Set texture wrapping parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    
    // Set texture filtering parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    GLenum format = GL_RGB;
    if (nrChannels == 1)
        format = GL_RED;
    else if (nrChannels == 3)
        format = GL_RGB;
    else if (nrChannels == 4)
        format = GL_RGBA;

    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    stbi_image_free(data);
    glBindTexture(GL_TEXTURE_2D, 0);

    return true;
}

void Texture::bind(unsigned int slot) const {
    if (m_id != 0) {
        glActiveTexture(GL_TEXTURE0 + slot);
        glBindTexture(GL_TEXTURE_2D, m_id);
    }
}

void Texture::unbind() const {
    glBindTexture(GL_TEXTURE_2D, 0);
}

void Texture::cleanup() {
    if (m_id != 0) {
        glDeleteTextures(1, &m_id);
        m_id = 0;
    }
}
