#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <string>
#include <stdexcept>

class Window {
public:
    Window(int width, int height, const std::string& title);
    ~Window();

    // Prevent copying
    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    bool shouldClose() const;
    void swapBuffers();
    void pollEvents();
    void setTitle(const std::string& title);
    void setVSync(bool enabled);
    bool isVSyncEnabled() const { return m_vsyncEnabled; }

    GLFWwindow* getGLFWWindow() const { return m_window; }
    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }

    double getMouseDeltaX() const { return m_mouseDeltaX; }
    double getMouseDeltaY() const { return m_mouseDeltaY; }
    double getScrollDeltaY() const { return m_scrollDeltaY; }
    void setCursorMode(int mode);

private:
    GLFWwindow* m_window;
    int m_width;
    int m_height;
    std::string m_title;

    double m_mouseX;
    double m_mouseY;
    double m_mouseDeltaX;
    double m_mouseDeltaY;
    double m_scrollDeltaY;
    bool m_firstMouse;
    bool m_vsyncEnabled;

    static void framebufferSizeCallback(GLFWwindow* window, int width, int height);
    static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void mouseCallback(GLFWwindow* window, double xpos, double ypos);
    static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);
    static void errorCallback(int error, const char* description);
};
