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
    PhysicalDeviceBuilder& setVulkan11Features(const VkPhysicalDeviceVulkan11Features& features);
    PhysicalDeviceBuilder& setVulkan12Features(const VkPhysicalDeviceVulkan12Features& features);
    PhysicalDeviceBuilder& setVulkan13Features(const VkPhysicalDeviceVulkan13Features& features);
  
    PhysicalDevice* build();

private:
    VkInstance m_Instance{ VK_NULL_HANDLE };
    VkSurfaceKHR m_Surface{ VK_NULL_HANDLE };
    std::vector<const char*> m_RequiredExtensions;
    VkPhysicalDeviceFeatures m_RequiredFeatures{};

    VkPhysicalDeviceVulkan11Features m_Vulkan11Features{};
    VkPhysicalDeviceVulkan12Features m_Vulkan12Features{};
    VkPhysicalDeviceVulkan13Features m_Vulkan13Features{};

    bool m_UseVulkan11Features{ false };
    bool m_UseVulkan12Features{ false };
    bool m_UseVulkan13Features{ false };
};
