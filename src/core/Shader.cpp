#include "core/Shader.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <stdexcept>
#include <glm/gtc/type_ptr.hpp>

Shader::Shader(const char* vertexPath, const char* fragmentPath) : ID(0) {
    std::string vertexCode;
    std::string fragmentCode;
    std::ifstream vShaderFile;
    std::ifstream fShaderFile;

    // Open the files
    vShaderFile.open(vertexPath);
    fShaderFile.open(fragmentPath);

    if (!vShaderFile.is_open()) {
        std::cerr << "ERROR::SHADER::FILE_NOT_FOUND: Failed to open vertex shader file: " << vertexPath << std::endl;
        throw std::runtime_error("Failed to open vertex shader: " + std::string(vertexPath));
    }
    if (!fShaderFile.is_open()) {
        std::cerr << "ERROR::SHADER::FILE_NOT_FOUND: Failed to open fragment shader file: " << fragmentPath << std::endl;
        throw std::runtime_error("Failed to open fragment shader: " + std::string(fragmentPath));
    }

    std::stringstream vShaderStream, fShaderStream;
    // Read file's buffer contents into streams
    vShaderStream << vShaderFile.rdbuf();
    fShaderStream << fShaderFile.rdbuf();

    // Close file handlers
    vShaderFile.close();
    fShaderFile.close();

    // Convert stream into string
    vertexCode = vShaderStream.str();
    fragmentCode = fShaderStream.str();

    const char* vShaderCode = vertexCode.c_str();
    const char* fShaderCode = fragmentCode.c_str();

    // Compile shaders
    unsigned int vertex = 0, fragment = 0;

    // Vertex shader compilation
    vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vShaderCode, NULL);
    glCompileShader(vertex);
    try {
        checkCompileErrors(vertex, "VERTEX");
    } catch (...) {
        glDeleteShader(vertex);
        throw;
    }

    // Fragment shader compilation
    fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fShaderCode, NULL);
    glCompileShader(fragment);
    try {
        checkCompileErrors(fragment, "FRAGMENT");
    } catch (...) {
        glDeleteShader(vertex);
        glDeleteShader(fragment);
        throw;
    }

    // Shader program linking
    ID = glCreateProgram();
    glAttachShader(ID, vertex);
    glAttachShader(ID, fragment);
    glLinkProgram(ID);
    try {
        checkCompileErrors(ID, "PROGRAM");
    } catch (...) {
        glDeleteShader(vertex);
        glDeleteShader(fragment);
        glDeleteProgram(ID);
        ID = 0;
        throw;
    }

    // Delete the shaders as they're linked into our program now and no longer necessary
    glDeleteShader(vertex);
    glDeleteShader(fragment);
}

Shader::~Shader() {
    if (ID != 0) {
        glDeleteProgram(ID);
    }
}

void Shader::use() const {
    glUseProgram(ID);
}

void Shader::setBool(const std::string &name, bool value) const {
    glUniform1i(glGetUniformLocation(ID, name.c_str()), (int)value);
}

void Shader::setInt(const std::string &name, int value) const {
    glUniform1i(glGetUniformLocation(ID, name.c_str()), value);
}

void Shader::setFloat(const std::string &name, float value) const {
    glUniform1f(glGetUniformLocation(ID, name.c_str()), value);
}

void Shader::setVec3(const std::string &name, const glm::vec3 &value) const {
    glUniform3fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
}

void Shader::setVec3(const std::string &name, float x, float y, float z) const {
    glUniform3f(glGetUniformLocation(ID, name.c_str()), x, y, z);
}

void Shader::setVec4(const std::string &name, const glm::vec4 &value) const {
    glUniform4fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
}

void Shader::setVec4(const std::string &name, float x, float y, float z, float w) const {
    glUniform4f(glGetUniformLocation(ID, name.c_str()), x, y, z, w);
}

void Shader::setMat4(const std::string &name, const glm::mat4 &mat) const {
    glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, glm::value_ptr(mat));
}

void Shader::checkCompileErrors(unsigned int shader, const std::string& type) const {
    int success;
    char infoLog[1024];
    if (type != "PROGRAM") {
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(shader, 1024, NULL, infoLog);
            std::cerr << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n"
                      << infoLog << "\n-------------------------------------------------\n"
                      << std::endl;
            throw std::runtime_error("Shader compilation failed for " + type);
        }
    } else {
        glGetProgramiv(shader, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(shader, 1024, NULL, infoLog);
            std::cerr << "ERROR::PROGRAM_LINKING_ERROR of type: " << type << "\n"
                      << infoLog << "\n-------------------------------------------------\n"
                      << std::endl;
            throw std::runtime_error("Shader program linking failed");
        }
    }
}
