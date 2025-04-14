#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

class Window
{
public:
    Window(int width, int height, const char* title);
    ~Window();

    GLFWwindow* getGLFWwindow() const { return m_Window; }
    bool shouldClose() const { return glfwWindowShouldClose(m_Window); }
    void pollEvents() const { glfwPollEvents(); }
    void getFramebufferSize(int& width, int& height) const { glfwGetFramebufferSize(m_Window, &width, &height); }
    bool isFramebufferResized() const { return framebufferResized; }
    void resetFramebufferResized() { framebufferResized = false; }

private:
    static void framebufferResizeCallback(GLFWwindow* window, int width, int height);

    GLFWwindow* m_Window;
    bool framebufferResized = false;
};