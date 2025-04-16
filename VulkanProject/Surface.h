#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

class Surface 
{
public:
    Surface(VkInstance instance, GLFWwindow* window);
    ~Surface();

    VkSurfaceKHR get() const;

private:
    VkInstance m_Instance;
    VkSurfaceKHR m_Surface;
};
