#include "Window.h"
#include <iostream>

Window::Window(int width, int height, const std::string& title)
    : m_window(nullptr), m_width(width), m_height(height), m_title(title) {
    
    // Set static GLFW error callback
    glfwSetErrorCallback(Window::errorCallback);

    // Initialize GLFW
    if (!glfwInit()) {
        throw std::runtime_error("Failed to initialize GLFW");
    }

    // Configure GLFW hints for OpenGL 3.3 Core Profile
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // Required for macOS
#endif

    // Create the GLFW window
    m_window = glfwCreateWindow(m_width, m_height, m_title.c_str(), nullptr, nullptr);
    if (!m_window) {
        glfwTerminate();
        throw std::runtime_error("Failed to create GLFW window");
    }

    // Set the user pointer to this Window instance so callbacks can access it
    glfwSetWindowUserPointer(m_window, this);

    // Make the OpenGL context current
    glfwMakeContextCurrent(m_window);

    // Set callbacks
    glfwSetFramebufferSizeCallback(m_window, Window::framebufferSizeCallback);
    glfwSetKeyCallback(m_window, Window::keyCallback);

    // Initialize GLAD (must be done after making context current)
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        glfwDestroyWindow(m_window);
        glfwTerminate();
        throw std::runtime_error("Failed to initialize GLAD");
    }

    // Set default viewport
    glViewport(0, 0, m_width, m_height);

    std::cout << "GLFW Window and OpenGL context initialized successfully.\n";
    std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << std::endl;
}

Window::~Window() {
    if (m_window) {
        glfwDestroyWindow(m_window);
    }
    glfwTerminate();
    std::cout << "GLFW terminated and window resources cleaned up.\n";
}

bool Window::shouldClose() const {
    return glfwWindowShouldClose(m_window);
}

void Window::swapBuffers() {
    glfwSwapBuffers(m_window);
}

void Window::pollEvents() {
    glfwPollEvents();
}

void Window::framebufferSizeCallback(GLFWwindow* window, int width, int height) {
    Window* win = static_cast<Window*>(glfwGetWindowUserPointer(window));
    if (win) {
        win->m_width = width;
        win->m_height = height;
        glViewport(0, 0, width, height);
        std::cout << "Window resized to: " << width << "x" << height << std::endl;
    }
}

void Window::keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    (void)scancode;
    (void)mods;
    
    Window* win = static_cast<Window*>(glfwGetWindowUserPointer(window));
    if (win) {
        // Pressing ESC closes the window
        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
            std::cout << "ESC pressed. Closing application...\n";
        }
    }
}

void Window::errorCallback(int error, const char* description) {
    std::cerr << "GLFW Error (" << error << "): " << description << std::endl;
}
