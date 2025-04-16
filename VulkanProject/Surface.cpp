#include "Surface.h"
#include <stdexcept>
#include <spdlog/spdlog.h>

Surface::Surface(VkInstance vkInstance, GLFWwindow* window)
    : m_Instance(vkInstance), m_Surface(VK_NULL_HANDLE)
{
    if (glfwCreateWindowSurface(m_Instance, window, nullptr, &m_Surface) != VK_SUCCESS) 
    {
        throw std::runtime_error("Failed to create window surface!");
    }

	spdlog::debug("Window surface created: {}", static_cast<void*>(m_Surface));
}

Surface::~Surface() 
{
    vkDestroySurfaceKHR(m_Instance, m_Surface, nullptr);

	spdlog::debug("Window surface destroyed.");
}

VkSurfaceKHR Surface::get() const 
{
    return m_Surface;
}
