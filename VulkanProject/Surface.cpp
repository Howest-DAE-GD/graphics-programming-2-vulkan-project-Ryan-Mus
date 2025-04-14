#include "Surface.h"
#include <stdexcept>

Surface::Surface(VkInstance vkInstance, GLFWwindow* window)
    : instance(vkInstance), surface(VK_NULL_HANDLE) {
    if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create window surface!");
    }
}

Surface::~Surface() {
    vkDestroySurfaceKHR(instance, surface, nullptr);
}

VkSurfaceKHR Surface::get() const {
    return surface;
}
