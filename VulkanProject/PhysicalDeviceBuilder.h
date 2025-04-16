#pragma once

#include <vulkan/vulkan.h>
#include <vector>

class PhysicalDevice;

class PhysicalDeviceBuilder 
{
public:
    PhysicalDeviceBuilder& setInstance(VkInstance instance);
    PhysicalDeviceBuilder& setSurface(VkSurfaceKHR surface);
    PhysicalDeviceBuilder& addRequiredExtension(const char* extension);
    PhysicalDeviceBuilder& setRequiredDeviceFeatures(const VkPhysicalDeviceFeatures& features);
    PhysicalDevice* build();

private:
    VkInstance m_Instance{ VK_NULL_HANDLE };
    VkSurfaceKHR m_Surface{ VK_NULL_HANDLE };
    std::vector<const char*> m_RequiredExtensions;
    VkPhysicalDeviceFeatures m_RequiredFeatures{};
};
