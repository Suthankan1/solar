#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <string>

class Shader {
public:
    // The program ID
    unsigned int ID;

    // Constructor reads and builds the shader from external files
    Shader(const char* vertexPath, const char* fragmentPath);
    ~Shader();

    // Activate/use the shader program
    void use() const;

    // Utility uniform setter functions
    void setBool(const std::string &name, bool value) const;
    void setInt(const std::string &name, int value) const;
    void setFloat(const std::string &name, float value) const;
    void setVec3(const std::string &name, const glm::vec3 &value) const;
    void setVec3(const std::string &name, float x, float y, float z) const;
    void setMat4(const std::string &name, const glm::mat4 &mat) const;

private:
    // Utility function for checking shader compilation/linking errors
    void checkCompileErrors(unsigned int shader, const std::string& type) const;
};
