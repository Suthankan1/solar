#pragma once
#include <string>

class Texture {
public:
    Texture();
    ~Texture();

    // Prevent copying
    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;

    // Allow moving
    Texture(Texture&& other) noexcept;
    Texture& operator=(Texture&& other) noexcept;

    bool load(const std::string& path);
    void bind(unsigned int slot = 0) const;
    void unbind() const;
    void cleanup();

    unsigned int getID() const { return m_id; }
    bool isValid() const { return m_id != 0; }

private:
    unsigned int m_id;
};
