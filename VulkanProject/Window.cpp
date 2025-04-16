#include "Window.h"
#include <stdexcept>
#include <spdlog/spdlog.h>

Window::Window(int width, int height, const char* title)
	: m_Width(width), m_Height(height)
{
    if (!glfwInit())
    {
        throw std::runtime_error("Failed to initialize GLFW");
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    m_Window = glfwCreateWindow(width, height, title, nullptr, nullptr);
    if (!m_Window)
    {
        glfwTerminate();
        throw std::runtime_error("Failed to create GLFW window");
    }

    glfwSetWindowUserPointer(m_Window, this);
    glfwSetFramebufferSizeCallback(m_Window, framebufferResizeCallback);

	spdlog::info("Window \"{}\" created with size : {}x{}", title, width, height);
}

Window::~Window()
{
    glfwDestroyWindow(m_Window);
    glfwTerminate();
	spdlog::debug("Window destroyed.");
}

void Window::framebufferResizeCallback(GLFWwindow* window, int width, int height)
{
    auto app = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
    app->m_FramebufferResized = true;

	spdlog::debug("FramebufferResizeCallback with size {}x{}", width, height);
}